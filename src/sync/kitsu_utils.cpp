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

#include "kitsu_utils.hpp"

#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <QUrlQuery>
#include <format>

#include "base/string.hpp"
#include "media/anime_season.hpp"
#include "sync/kitsu_parsers.hpp"
#include "sync/kitsu_ratings.hpp"

namespace sync::kitsu {

QString fromSeasonName(const anime::SeasonName name) {
  // clang-format off
  switch (name) {
    case anime::SeasonName::Unknown: return "";
    case anime::SeasonName::Winter: return "winter";
    case anime::SeasonName::Spring: return "spring";
    case anime::SeasonName::Summer: return "summer";
    case anime::SeasonName::Fall: return "fall";
  }
  // clang-format on
  return "";
}

////////////////////////////////////////////////////////////////////////////////

QJsonObject buildLibraryEntryObject(const anime::list::Entry& entry,
                                    const anime::list::Fields dirty, const QString& userId) {
  using anime::list::Field;

  QJsonObject attributes;

  // Note that we send `null` (rather than `0` or `""`) to remove certain values.
  if (dirty & Field::Episode) {
    attributes["progress"] = entry.watched_episodes;
  }
  if (dirty & Field::Score) {
    attributes["ratingTwenty"] =
        entry.score > 0 ? QJsonValue(fromListScore(entry.score)) : QJsonValue{QJsonValue::Null};
  }
  if (dirty & Field::Status) {
    attributes["status"] = fromListStatus(entry.status);
  }
  if (dirty & Field::Rewatching) {
    attributes["reconsuming"] = entry.rewatching;
  }
  if (dirty & Field::RewatchedTimes) {
    attributes["reconsumeCount"] = entry.rewatched_times;
  }
  if (dirty & Field::DateStarted) {
    attributes["startedAt"] =
        entry.date_started ? fromListDate(QString::fromStdString(entry.date_started.to_string()))
                           : QJsonValue{QJsonValue::Null};
  }
  if (dirty & Field::DateCompleted) {
    attributes["finishedAt"] =
        entry.date_completed
            ? fromListDate(QString::fromStdString(entry.date_completed.to_string()))
            : QJsonValue{QJsonValue::Null};
  }
  if (dirty & Field::Notes) {
    attributes["notes"] = QString::fromStdString(entry.notes);
  }

  QJsonObject animeData{
      {"type", "anime"},
      {"id", QString::number(entry.anime_id)},
  };

  QJsonObject userData{
      {"type", "users"},
      {"id", userId},
  };

  QJsonObject data{
      {"type", "libraryEntries"},
      {"attributes", attributes},
      {"relationships",
       QJsonObject{
           {"anime", QJsonObject{{"data", animeData}}},
           {"user", QJsonObject{{"data", userData}}},
       }},
  };

  if (entry.id != anime::list::kUnknownId) {
    data["id"] = QString::number(entry.id);
  }

  return QJsonObject{{"data", data}};
}

QByteArray formUrlEncode(const QUrlQuery& query) {
  return query.toString(QUrl::FullyEncoded).toUtf8();
}

std::optional<int> pagingOffset(const QJsonObject& links, const QString& key) {
  const QUrlQuery query{QUrl{links[key].toString()}};

  if (!query.hasQueryItem(u"page[offset]"_s)) return std::nullopt;

  bool ok = false;
  const auto offset = query.queryItemValue(u"page[offset]"_s).toInt(&ok);
  return ok ? std::make_optional(offset) : std::nullopt;
}

////////////////////////////////////////////////////////////////////////////////

std::string animePageUrl(const int id) {
  return std::format("https://kitsu.app/anime/{}", id);
}

}  // namespace sync::kitsu
