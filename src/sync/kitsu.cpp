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
#include <QRestReply>
#include <QUrlQuery>

#include "base/string.hpp"
#include "media/anime_db.hpp"
#include "sync/kitsu_error.hpp"
#include "sync/kitsu_parsers.hpp"
#include "sync/kitsu_utils.hpp"
#include "taiga/accounts.hpp"

// Kitsu API documentation:
// https://kitsu.docs.apiary.io

namespace sync::kitsu {

namespace {

constexpr int kLibraryPageLimit = 500;

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
  const QUrlQuery query{{"include", "categories,animeProductions,animeProductions.producer"}};

  const auto callback = [this, id](QRestReply& reply) {
    if (isError(reply)) {
      if (retryOnTokenExpiry(reply, [this, id] { fetchAnime(id); })) return;
      handleError(*this, reply);
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

void Service::fetchListEntries(const int offset) {
  // Library entries are filtered by numeric user ID rather than username, so it must be
  // resolved first.
  if (taiga::accounts.kitsuUserId().empty()) {
    resolveUser([this, offset] { fetchListEntries(offset); });
    return;
  }

  const QUrlQuery query{{
      {"filter[user_id]", QString::fromStdString(taiga::accounts.kitsuUserId())},
      {"filter[kind]", "anime"},
      {"include", "anime"},
      {"page[offset]", QString::number(offset)},
      {"page[limit]", QString::number(kLibraryPageLimit)},
  }};

  const auto callback = [this, offset](QRestReply& reply) {
    if (isError(reply)) {
      if (retryOnTokenExpiry(reply, [this, offset] { fetchListEntries(offset); })) return;
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
      }
    }

    if (const auto nextOffset = pagingOffset(root["links"].toObject(), u"next"_s)) {
      fetchListEntries(*nextOffset);
    }
  };

  manager_.get(api_.createRequest(u"/library-entries"_s, query), this, callback);
}

}  // namespace sync::kitsu
