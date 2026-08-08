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

#include "kitsu_parsers.hpp"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>

#include "base/chrono.hpp"
#include "media/anime.hpp"
#include "media/anime_list.hpp"
#include "sync/kitsu_ratings.hpp"
#include "sync/service.hpp"

namespace sync::kitsu {

namespace {

FuzzyDate parseDate(const QString& value) {
  return value.size() >= 10 ? FuzzyDate(value.first(10).toStdString()) : FuzzyDate{};
}

}  // namespace

anime::AgeRating parseAgeRating(const QString& value) {
  static const QMap<QString, anime::AgeRating> table{
      {"G", anime::AgeRating::G},
      {"PG", anime::AgeRating::PG},
      {"R", anime::AgeRating::R17},
      {"R18", anime::AgeRating::R18},
  };
  return table.value(value.toUpper(), anime::AgeRating::Unknown);
}

float parseScore(const QString& value) {
  return value.toFloat() / 10.0f;
}

float fromScore(float value) {
  return value * 10.0f;
}

anime::Status parseStatus(const QString& value) {
  // clang-format off
  static const QMap<QString, anime::Status> table{
      {"current", anime::Status::Airing},
      {"finished", anime::Status::FinishedAiring},
      {"tba", anime::Status::NotYetAired},
      {"unreleased", anime::Status::NotYetAired},
      {"upcoming", anime::Status::NotYetAired},
  };
  // clang-format on
  return table.value(value, anime::Status::Unknown);
}

anime::Type parseType(const QString& value) {
  // clang-format off
  static const QMap<QString, anime::Type> table{
      {"TV", anime::Type::Tv},
      {"special", anime::Type::Special},
      {"OVA", anime::Type::Ova},
      {"ONA", anime::Type::Ona},
      {"movie", anime::Type::Movie},
      {"music", anime::Type::Music},
  };
  // clang-format on
  return table.value(value, anime::Type::Unknown);
}

QString parseListDate(const QString& value) {
  return value.size() >= 10 ? value.first(10) : QString{};
}

QString fromListDate(const QString& value) {
  return value + "T00:00:00.000Z";
}

std::time_t parseListLastUpdated(const QString& value) {
  return QDateTime::fromString(value, Qt::DateFormat::ISODate).toSecsSinceEpoch();
}

anime::list::Status parseListStatus(const QString& value) {
  // clang-format off
  static const QMap<QString, anime::list::Status> table{
      {"current", anime::list::Status::Watching},
      {"planned", anime::list::Status::PlanToWatch},
      {"completed", anime::list::Status::Completed},
      {"on_hold", anime::list::Status::OnHold},
      {"dropped", anime::list::Status::Dropped},
  };
  // clang-format on
  return table.value(value, anime::list::Status::NotInList);
}

QString fromListStatus(const anime::list::Status value) {
  // clang-format off
  switch (value) {
    case anime::list::Status::Watching: return "current";
    case anime::list::Status::Completed: return "completed";
    case anime::list::Status::OnHold: return "on_hold";
    case anime::list::Status::Dropped: return "dropped";
    case anime::list::Status::PlanToWatch: return "planned";
  }
  // clang-format on
  return "";
}

////////////////////////////////////////////////////////////////////////////////

std::optional<anime::Details> parseAnime(const QJsonValue& data, const QJsonArray& included) {
  const int id = data["id"].toVariant().toInt();

  if (!id) return std::nullopt;

  const auto attributes = data["attributes"].toObject();
  const auto titles = attributes["titles"].toObject();

  anime::Details item{
      .id = id,
      .last_modified = QDateTime::currentSecsSinceEpoch(),
      .episode_count = attributes["episodeCount"].toInt(),
      .episode_length = attributes["episodeLength"].toInt(),
      .age_rating = parseAgeRating(attributes["ageRating"].toString()),
      .status = parseStatus(attributes["status"].toString()),
      .type = parseType(attributes["subtype"].toString()),
      .date_started = parseDate(attributes["startDate"].toString()),
      .date_finished = parseDate(attributes["endDate"].toString()),
      .score = parseScore(attributes["averageRating"].toVariant().toString()),
      .popularity_rank = attributes["popularityRank"].toInt(),
      .image_url = attributes["posterImage"]["small"].toString().toStdString(),
      .slug = attributes["slug"].toString().toStdString(),
      .synopsis = attributes["synopsis"].toString().toStdString(),
      .trailer_id = attributes["youtubeVideoId"].toString().toStdString(),
      .titles{
          .romaji = titles["en_jp"].toString().toStdString(),
          .english = titles["en"].toString().toStdString(),
          .japanese = titles["ja_jp"].toString().toStdString(),
      },
  };

  if (item.titles.romaji.empty()) {
    item.titles.romaji = attributes["canonicalTitle"].toString().toStdString();
  }

  for (const auto& value : attributes["abbreviatedTitles"].toArray()) {
    if (auto synonym = value.toString().toStdString(); !synonym.empty()) {
      item.titles.synonyms.push_back(std::move(synonym));
    }
  }

  for (const auto& value : included) {
    const auto resource = value.toObject();
    const auto type = resource["type"].toString();
    if (type == "categories") {
      item.genres.push_back(resource["attributes"]["title"].toString().toStdString());
    } else if (type == "producers") {
      item.producers.push_back(resource["attributes"]["name"].toString().toStdString());
    }
  }

  return item;
}

std::optional<anime::list::Entry> parseListEntry(const QJsonValue& json, const int animeId) {
  if (!animeId) return std::nullopt;

  const auto attributes = json["attributes"].toObject();

  return anime::list::Entry{
      .id = json["id"].toVariant().toLongLong(),
      .anime_id = animeId,
      .watched_episodes = attributes["progress"].toInt(),
      .score = parseListScore(attributes["ratingTwenty"].toInt()),
      .status = parseListStatus(attributes["status"].toString()),
      .is_private = attributes["private"].toBool(),
      .rewatched_times = attributes["reconsumeCount"].toInt(),
      .rewatching = attributes["reconsuming"].toBool(),
      .date_started = parseDate(parseListDate(attributes["startedAt"].toString())),
      .date_completed = parseDate(parseListDate(attributes["finishedAt"].toString())),
      .last_updated = parseListLastUpdated(attributes["updatedAt"].toString()),
      .notes = attributes["notes"].toString().toStdString(),
  };
}

}  // namespace sync::kitsu
