/**
 * Taiga
 * Copyright (C) 2010-2026, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "kitsu.hpp"

#include <QHttpHeaders>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QRestReply>
#include <QUrlQuery>
#include <ranges>

#include "base/string.hpp"
#include "media/anime_db.hpp"
#include "sync/kitsu/kitsu_error.hpp"
#include "sync/kitsu/kitsu_parsers.hpp"
#include "sync/kitsu/kitsu_utils.hpp"
#include "sync/queue.hpp"
#include "taiga/accounts.hpp"

// Kitsu API documentation:
// https://kitsu.docs.apiary.io

namespace sync::kitsu {

namespace {

constexpr int kLibraryPageLimit = 500;

// Kitsu's JSON:API configuration sets a maximum page size of 20 on this resource. Asking for
// more results returns an error: "Limit exceeds maximum page size of 20."
constexpr int kSearchPageLimit = 20;

}  // namespace

Service::Service() : sync::Service{ServiceId::Kitsu} {
  api_.setBaseUrl(QUrl{kApiUrl});

  auto headers = api_.commonHeaders();
  headers.append(QHttpHeaders::WellKnownHeader::Accept, kJsonApiMediaType);
  api_.setCommonHeaders(headers);

  if (const auto token = taiga::accounts.kitsuAccessToken(); !token.empty()) {
    api_.setBearerToken(QByteArray::fromStdString(token));
  }
}

Service* Service::instance() {
  static auto service = new Service();
  return service;
}

////////////////////////////////////////////////////////////////////////////////

void Service::fetchAnime(const int id) {
  const QUrlQuery query{{
      {"include", "categories,animeProductions,animeProductions.producer"},
      {"fields[anime]", animeFields()},
      {"fields[animeProductions]", "producer"},
      {"fields[categories]", "title"},
      {"fields[producers]", "name"},
  }};

  const auto callback = [this, id](QRestReply& reply) {
    if (isError(reply)) {
      if (retryOnTokenExpiry(reply, [this, id] { fetchAnime(id); })) return;
      if (reply.httpStatus() == 404) {
        sync::invalidateAnime(id);
      } else {
        handleError(*this, reply);
      }
      return;
    }

    const auto json = reply.readJson();
    if (!json) {
      handleError(*this, reply, "Could not parse anime object.");
      return;
    }

    const auto root = json->object();
    const auto item = parseAnime(root["data"], root["included"].toArray());

    if (!item) {
      handleError(*this, reply, "Could not parse anime object.");
      return;
    }

    anime::db.updateItem(*item);
  };

  manager_.get(api_.createRequest(u"/anime/%1"_s.arg(id), query), this, callback);
}

void Service::fetchListEntries(const int offset, QSet<int> fetchedIds) {
  // Library entries are filtered by numeric user ID rather than username, so it must be
  // resolved first.
  if (taiga::accounts.kitsuUserId().empty()) {
    resolveUser([this, offset, fetchedIds] { fetchListEntries(offset, fetchedIds); });
    return;
  }

  const QUrlQuery query{{
      {"filter[user_id]", QString::fromStdString(taiga::accounts.kitsuUserId())},
      {"filter[kind]", "anime"},
      {"include", "anime"},
      {"page[offset]", QString::number(offset)},
      {"page[limit]", QString::number(kLibraryPageLimit)},
      {"fields[anime]", animeFields(true)},
      {"fields[libraryEntries]", libraryEntryFields()},
  }};

  const auto callback = [this, offset, fetchedIds](QRestReply& reply) mutable {
    if (isError(reply)) {
      if (retryOnTokenExpiry(
              reply, [this, offset, fetchedIds] { fetchListEntries(offset, fetchedIds); })) {
        return;
      }
      handleError(*this, reply);
      return;
    }

    const auto json = reply.readJson();
    if (!json) {
      handleError(*this, reply, "Could not parse anime list.");
      return;
    }

    const auto root = json->object();

    for (const auto& value : root["included"].toArray()) {
      if (const auto item = parseAnime(value)) {
        anime::db.updateItem(*item);
      }
    }

    for (const auto& value : root["data"].toArray()) {
      const auto entryObject = value.toObject();
      const auto animeId = entryObject["relationships"]["anime"]["data"]["id"].toVariant().toInt();
      if (const auto entry = parseListEntry(entryObject, animeId)) {
        anime::db.updateEntry(*entry);
        fetchedIds.insert(animeId);
      }
    }

    if (const auto nextOffset = pagingOffset(root["links"].toObject(), u"next"_s)) {
      fetchListEntries(*nextOffset, fetchedIds);
    } else {
      sync::pruneMissingEntries(fetchedIds);
    }
  };

  manager_.get(api_.createRequest(u"/library-entries"_s, query), this, callback);
}

void Service::search(const SearchParams& params, const int offset) {
  QUrlQuery query;

  if (!params.text.isEmpty()) {
    query.addQueryItem(u"filter[text]"_s, params.text);
  }
  if (params.season) {
    query.addQueryItem(u"filter[season]"_s, fromSeasonName(*params.season));
  }
  if (params.year) {
    query.addQueryItem(u"filter[season_year]"_s, QString::number(*params.year));
  }
  if (params.type) {
    query.addQueryItem(u"filter[subtype]"_s, fromType(*params.type));
  }
  if (params.status) {
    query.addQueryItem(u"filter[status]"_s, fromStatus(*params.status));
  }
  if (const auto sort = fromSearchParams(params); !sort.isEmpty()) {
    query.addQueryItem(u"sort"_s, sort);
  }
  query.addQueryItem(u"page[offset]"_s, QString::number(offset));
  query.addQueryItem(u"page[limit]"_s, QString::number(kSearchPageLimit));
  query.addQueryItem(u"fields[anime]"_s, animeFields());

  const auto callback = [this, params, offset](QRestReply& reply) {
    if (isError(reply)) {
      if (retryOnTokenExpiry(reply, [this, params, offset] { search(params, offset); })) return;
      handleError(*this, reply);
      emit searchCompleted(params, {});
      return;
    }

    const auto json = reply.readJson();
    if (!json) {
      handleError(*this, reply, "Could not parse search results.");
      emit searchCompleted(params, {});
      return;
    }

    const auto root = json->object();

    QList<int> ids;
    for (const auto& value : root["data"].toArray()) {
      if (const auto item = parseAnime(value)) {
        anime::db.updateItem(*item);
        ids.append(item->id);
      }
    }

    emit searchCompleted(params, ids);

    // Auto-paginate only for season browsing.
    const bool seasonScoped = params.season.has_value() && params.year.has_value();
    if (seasonScoped) {
      if (const auto nextOffset = pagingOffset(root["links"].toObject(), u"next"_s)) {
        search(params, *nextOffset);
      }
    }
  };

  manager_.get(api_.createRequest(u"/anime"_s, query), this, callback);
}

void Service::addListEntry(const int id, const anime::list::Fields dirty) {
  const auto listEntry = anime::db.entry(id);
  if (!listEntry) return;

  const QUrlQuery query{{"fields[libraryEntries]", libraryEntryFields()}};
  auto request = api_.createRequest(u"/library-entries"_s, query);
  request.setHeader(QNetworkRequest::ContentTypeHeader, kJsonApiMediaType);

  const auto body = buildLibraryEntryObject(*listEntry, dirty,
                                            QString::fromStdString(taiga::accounts.kitsuUserId()));

  const auto callback = [this, id, dirty](QRestReply& reply) {
    const auto json = reply.readJson();

    // Kitsu returns 422 if the anime is already in the user's list. Treat this as a
    // successful (idempotent) add rather than an error.
    if (reply.httpStatus() == 422) {
      const auto errors = json ? json->object()["errors"].toArray() : QJsonArray{};
      const bool duplicate = std::ranges::any_of(errors, [](const QJsonValue& value) {
        return value["detail"].toString().contains(u"has already been taken"_s);
      });
      if (duplicate) {
        sync::queue.complete(true);
        return;
      }
    }

    if (isError(reply)) {
      if (retryOnTokenExpiry(reply, [this, id, dirty] { addListEntry(id, dirty); })) return;
      handleError(*this, reply, json);
      sync::queue.complete(false, "Failed to add list entry.");
      return;
    }

    if (const auto entry =
            json ? parseListEntry(json->object()["data"].toObject(), id) : std::nullopt) {
      anime::db.updateEntry(*entry);
    }

    sync::queue.complete(true);
  };

  manager_.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact), this, callback);
}

void Service::updateListEntry(const int id, const anime::list::Fields dirty) {
  const auto listEntry = anime::db.entry(id);
  if (!listEntry) return;

  const QUrlQuery query{{"fields[libraryEntries]", libraryEntryFields()}};
  auto request = api_.createRequest(u"/library-entries/%1"_s.arg(listEntry->id), query);
  request.setHeader(QNetworkRequest::ContentTypeHeader, kJsonApiMediaType);

  const auto body = buildLibraryEntryObject(*listEntry, dirty,
                                            QString::fromStdString(taiga::accounts.kitsuUserId()));

  const auto callback = [this, id, dirty](QRestReply& reply) {
    if (isError(reply)) {
      if (retryOnTokenExpiry(reply, [this, id, dirty] { updateListEntry(id, dirty); })) return;
      handleError(*this, reply,
                  reply.httpStatus() == 404 ? u"Anime list entry does not exist."_s : QString{});
      sync::queue.complete(false, "Failed to update list entry.");
      return;
    }

    const auto json = reply.readJson();
    if (const auto entry =
            json ? parseListEntry(json->object()["data"].toObject(), id) : std::nullopt) {
      anime::db.updateEntry(*entry);
    }

    sync::queue.complete(true);
  };

  manager_.patch(request, QJsonDocument(body).toJson(QJsonDocument::Compact), this, callback);
}

void Service::deleteListEntry(const int id) {
  const auto listEntry = anime::db.entry(id);
  if (!listEntry) return;

  const auto callback = [this, id](QRestReply& reply) {
    if (isError(reply) && reply.httpStatus() != 404) {
      if (retryOnTokenExpiry(reply, [this, id] { deleteListEntry(id); })) return;
      handleError(*this, reply);
      sync::queue.complete(false, "Failed to delete list entry.");
      return;
    }

    anime::db.deleteEntry(id);
    sync::queue.complete(true);
  };

  manager_.deleteResource(api_.createRequest(u"/library-entries/%1"_s.arg(listEntry->id)), this,
                          callback);
}

}  // namespace sync::kitsu
