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
#include "media/anime.hpp"
#include "media/anime_season.hpp"
#include "sync/kitsu_parsers.hpp"
#include "sync/kitsu_ratings.hpp"
#include "sync/search_params.hpp"

namespace sync::kitsu {

QString fromSearchParams(const SearchParams& params) {
  // Text search already returns results in relevance order.
  if (!params.text.isEmpty()) return {};

  if (params.sort) {
    const auto prefix = params.sortOrder == Qt::SortOrder::DescendingOrder ? u"-"_s : QString{};
    // clang-format off
    switch (*params.sort) {
      case SearchSort::Title: return prefix + u"canonicalTitle"_s;
      case SearchSort::Duration: return prefix + u"episodeLength"_s;
      case SearchSort::Score: return prefix + u"averageRating"_s;
      case SearchSort::Type: return prefix + u"subtype"_s;
      case SearchSort::StartDate: return prefix + u"startDate"_s;
    }
    // clang-format on
  }

  // Without an explicit sort, season browsing returns inconsistent ordering
  // and duplicate objects across pages.
  return u"-user_count"_s;
}

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

QString fromStatus(const anime::Status value) {
  // clang-format off
  switch (value) {
    case anime::Status::Unknown: return "";
    case anime::Status::Airing: return "current";
    case anime::Status::FinishedAiring: return "finished";
    case anime::Status::NotYetAired: return "tba,unreleased,upcoming";
  }
  // clang-format on
  return "";
}

QString fromType(const anime::Type value) {
  // clang-format off
  switch (value) {
    case anime::Type::Unknown: return "";
    case anime::Type::Tv: return "TV";
    case anime::Type::Ova: return "OVA";
    case anime::Type::Movie: return "movie";
    case anime::Type::Special: return "special";
    case anime::Type::Ona: return "ONA";
    case anime::Type::Music: return "music";
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

QString animeFields(const bool minimal) {
  auto fields =
      // attributes
      u"abbreviatedTitles,"
      "ageRating,"
      "averageRating,"
      "canonicalTitle,"
      "endDate,"
      "episodeCount,"
      "episodeLength,"
      "popularityRank,"
      "posterImage,"
      "slug,"
      "startDate,"
      "status,"
      "subtype,"
      "titles,"
      "youtubeVideoId,"
      // relationships
      "animeProductions,"
      "categories"_s;

  if (!minimal) fields += u",synopsis"_s;

  return fields;
}

QString libraryEntryFields() {
  return
      // attributes
      u"finishedAt,"
      "notes,"
      "private,"
      "progress,"
      "ratingTwenty,"
      "reconsumeCount,"
      "reconsuming,"
      "startedAt,"
      "status,"
      "updatedAt,"
      // relationships
      "anime"_s;
}

QString userFields() {
  return u"email,"
         "name,"
         "ratingSystem,"
         "slug"_s;
}

std::string animePageUrl(const int id) {
  return std::format("https://kitsu.app/anime/{}", id);
}

}  // namespace sync::kitsu
