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

#include "myanimelist.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QRestReply>
#include <QUrlQuery>

#include "base/string.hpp"
#include "media/anime_db.hpp"
#include "media/anime_season.hpp"
#include "sync/myanimelist/myanimelist_error.hpp"
#include "sync/myanimelist/myanimelist_parsers.hpp"
#include "sync/myanimelist/myanimelist_utils.hpp"
#include "sync/queue.hpp"
#include "taiga/accounts.hpp"

// MyAnimeList API documentation:
// https://myanimelist.net/apiconfig/references/api/v2

namespace sync::myanimelist {

namespace {

constexpr int kLibraryPageLimit = 1000;
constexpr int kSearchPageLimit = 100;
constexpr int kSeasonPageLimit = 500;

}  // namespace

Service::Service() : sync::Service{ServiceId::MyAnimeList} {
  api_.setBaseUrl(QUrl{kApiUrl});

  if (const auto token = taiga::accounts.myanimelistAccessToken(); !token.empty()) {
    api_.setBearerToken(QByteArray::fromStdString(token));
  }
}

Service* Service::instance() {
  static auto service = new Service();
  return service;
}

////////////////////////////////////////////////////////////////////////////////

void Service::fetchAnime(const int id) {
  const QUrlQuery query{{"fields", animeFields()}};

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

    const auto item = reply.readJson().and_then(
        [](const QJsonDocument& json) { return parseAnime(json.object()); });

    if (!item) {
      handleError(*this, reply, "Could not parse anime object.");
      return;
    }

    anime::db.updateItem(*item);
  };

  manager_.get(api_.createRequest(u"/anime/%1"_s.arg(id), query), this, callback);
}

void Service::search(const SearchParams& params, const int offset) {
  const bool seasonScoped = params.season.has_value() && params.year.has_value();

  QString path;
  QUrlQuery query;

  if (seasonScoped) {
    path = u"/anime/season/%1/%2"_s.arg(*params.year).arg(fromSeasonName(*params.season));
    query.addQueryItem(u"limit"_s, QString::number(kSeasonPageLimit));
  } else {
    path = u"/anime"_s;
    query.addQueryItem(u"q"_s, params.text);
    query.addQueryItem(u"limit"_s, QString::number(kSearchPageLimit));
  }
  query.addQueryItem(u"offset"_s, QString::number(offset));
  query.addQueryItem(u"nsfw"_s, u"true"_s);
  query.addQueryItem(u"fields"_s, animeFields());

  const auto callback = [this, params, offset, seasonScoped](QRestReply& reply) {
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
      if (const auto item = parseAnime(value.toObject()["node"])) {
        anime::db.updateItem(*item);
        ids.append(item->id);
      }
    }

    emit searchCompleted(params, ids);

    // Auto-paginate only for season browsing.
    if (seasonScoped) {
      if (const auto nextOffset = pagingOffset(root["paging"].toObject(), u"next"_s)) {
        search(params, *nextOffset);
      }
    }
  };

  manager_.get(api_.createRequest(path, query), this, callback);
}

void Service::fetchListEntries(const int offset, QSet<int> fetchedIds) {
  const auto username = QString::fromStdString(taiga::accounts.myanimelistUsername());
  const auto path = u"/users/%1/animelist"_s.arg(username);

  const QUrlQuery query{
      {"limit", QString::number(kLibraryPageLimit)},
      {"offset", QString::number(offset)},
      {"nsfw", "true"},
      {"fields", u"%1,list_status{%2}"_s.arg(animeFields(), listStatusFields())},
  };

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

    for (const auto& value : root["data"].toArray()) {
      const auto entryValue = value.toObject();

      const auto item = parseAnime(entryValue["node"]);
      if (!item) continue;

      anime::db.updateItem(*item);

      if (const auto entry = parseListEntry(entryValue["list_status"], item->id)) {
        anime::db.updateEntry(*entry);
        fetchedIds.insert(item->id);
      }
    }

    if (const auto nextOffset = pagingOffset(root["paging"].toObject(), u"next"_s)) {
      fetchListEntries(*nextOffset, fetchedIds);
    } else {
      sync::pruneMissingEntries(fetchedIds);
    }
  };

  manager_.get(api_.createRequest(path, query), this, callback);
}

void Service::addListEntry(const int id, const anime::list::Fields dirty) {
  updateListEntry(id, dirty);
}

void Service::deleteListEntry(const int id) {
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

  manager_.deleteResource(api_.createRequest(u"/anime/%1/my_list_status"_s.arg(id)), this,
                          callback);
}

void Service::updateListEntry(const int id, const anime::list::Fields dirty) {
  const auto listEntry = anime::db.entry(id);

  if (!listEntry) return;

  using anime::list::Field;

  QUrlQuery body;
  if (dirty & Field::Episode) {
    body.addQueryItem(u"num_watched_episodes"_s, QString::number(listEntry->watched_episodes));
  }
  if (dirty & (Field::Status | Field::Rewatching)) {
    body.addQueryItem(u"status"_s, fromListStatus(listEntry->status));
    body.addQueryItem(u"is_rewatching"_s, listEntry->rewatching ? u"true"_s : u"false"_s);
  }
  if (dirty & Field::Score) {
    body.addQueryItem(u"score"_s, QString::number(fromListScore(listEntry->score)));
  }
  if (dirty & Field::RewatchedTimes) {
    body.addQueryItem(u"num_times_rewatched"_s, QString::number(listEntry->rewatched_times));
  }
  if (dirty & Field::DateStarted) {
    body.addQueryItem(u"start_date"_s, QString::fromStdString(listEntry->date_started.to_string()));
  }
  if (dirty & Field::DateCompleted) {
    body.addQueryItem(u"finish_date"_s,
                      QString::fromStdString(listEntry->date_completed.to_string()));
  }
  if (dirty & Field::Notes) {
    body.addQueryItem(u"comments"_s, QString::fromStdString(listEntry->notes));
  }

  auto request = api_.createRequest(u"/anime/%1/my_list_status"_s.arg(id));
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

  const auto callback = [this, id, dirty](QRestReply& reply) {
    if (isError(reply)) {
      if (retryOnTokenExpiry(reply, [this, id, dirty] { updateListEntry(id, dirty); })) return;
      handleError(*this, reply,
                  reply.httpStatus() == 404 ? u"Anime list entry does not exist."_s : QString{});
      sync::queue.complete(false, "Failed to update list entry.");
      return;
    }

    const auto json = reply.readJson();
    if (!json) {
      handleError(*this, reply, "Could not parse list entry.");
      sync::queue.complete(false, "Could not parse list entry.");
      return;
    }

    if (const auto entry = parseListEntry(json->object(), id)) {
      anime::db.updateEntry(*entry);
    }

    sync::queue.complete(true);
  };

  manager_.patch(request, formUrlEncode(body), this, callback);
}

}  // namespace sync::myanimelist
