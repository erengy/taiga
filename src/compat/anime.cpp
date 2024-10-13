/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
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

#include "anime.hpp"

#include <QFile>
#include <QRegularExpression>
#include <QXmlStreamReader>

namespace {

// This regex is used to remove the extra root element from v1's XML documents so that they
// can be read by `QXmlStreamReader` without an "Extra content at end of document" error.
// See #842 for more information.
static const QRegularExpression metaElementRegex{"<meta>.+</meta>",
                                                 QRegularExpression::DotMatchesEverythingOption};

}  // namespace

namespace compat::v1 {

QList<Anime> read_anime_database(const std::string& path) {
  QFile file(QString::fromStdString(path));

  if (!file.open(QIODevice::ReadOnly)) return {};

  QString str{file.readAll()};
  str.remove(metaElementRegex);

  QXmlStreamReader xml(str);

  if (!xml.readNextStartElement()) return {};
  if (xml.name() != u"database") return {};

  QList<Anime> data;

  while (xml.readNextStartElement()) {
    if (xml.name() != u"anime") break;

    Anime anime;

    static const auto to_vector = [](const QStringList& list) {
      std::vector<std::string> vector;
      for (const auto& str : list) {
        vector.push_back(str.toStdString());
      }
      return vector;
    };

    while (xml.readNextStartElement()) {
      if (xml.name() == u"id") {
        anime.id = xml.readElementText().toInt();
      } else if (xml.name() == u"title") {
        anime.titles.romaji = xml.readElementText().toStdString();
      } else if (xml.name() == u"english") {
        anime.titles.english = xml.readElementText().toStdString();
      } else if (xml.name() == u"japanese") {
        anime.titles.japanese = xml.readElementText().toStdString();
      } else if (xml.name() == u"synonym") {
        anime.titles.synonyms.push_back(xml.readElementText().toStdString());
      } else if (xml.name() == u"type") {
        anime.type = static_cast<anime::Type>(xml.readElementText().toInt());
      } else if (xml.name() == u"status") {
        anime.status = static_cast<anime::Status>(xml.readElementText().toInt());
      } else if (xml.name() == u"episode_count") {
        anime.episode_count = xml.readElementText().toInt();
      } else if (xml.name() == u"episode_length") {
        anime.episode_length = xml.readElementText().toInt();
      } else if (xml.name() == u"date_start") {
        anime.date_started = FuzzyDate(xml.readElementText().toStdString());
      } else if (xml.name() == u"date_end") {
        anime.date_finished = FuzzyDate(xml.readElementText().toStdString());
      } else if (xml.name() == u"image") {
        anime.image_url = xml.readElementText().toStdString();
      } else if (xml.name() == u"trailer_id") {
        anime.trailer_id = xml.readElementText().toStdString();
      } else if (xml.name() == u"age_rating") {
        anime.age_rating = static_cast<anime::AgeRating>(xml.readElementText().toInt());
      } else if (xml.name() == u"genres") {
        anime.genres = to_vector(xml.readElementText().split(", "));
      } else if (xml.name() == u"tags") {
        anime.tags = to_vector(xml.readElementText().split(", "));
      } else if (xml.name() == u"producers") {
        anime.producers = to_vector(xml.readElementText().split(", "));
      } else if (xml.name() == u"studios") {
        anime.studios = to_vector(xml.readElementText().split(", "));
      } else if (xml.name() == u"score") {
        anime.score = xml.readElementText().toFloat();
      } else if (xml.name() == u"popularity") {
        anime.popularity_rank = xml.readElementText().toInt();
      } else if (xml.name() == u"synopsis") {
        anime.synopsis = xml.readElementText().toStdString();
      } else if (xml.name() == u"last_aired_episode") {
        anime.last_aired_episode = xml.readElementText().toInt();
      } else if (xml.name() == u"next_episode_time") {
        anime.next_episode_time = xml.readElementText().toInt();
      } else if (xml.name() == u"modified") {
        anime.last_modified = xml.readElementText().toInt();
      } else {
        xml.skipCurrentElement();
      }
    }

    data.emplace_back(anime);
  }

  return data;
}

QList<ListEntry> read_list_entries(const std::string& path) {
  QFile file(QString::fromStdString(path));

  if (!file.open(QIODevice::ReadOnly)) return {};

  QString str{file.readAll()};
  str.remove(metaElementRegex);

  QXmlStreamReader xml(str);

  if (!xml.readNextStartElement()) return {};
  if (xml.name() != u"library") return {};

  QList<ListEntry> entries;

  while (xml.readNextStartElement()) {
    if (xml.name() != u"anime") break;

    ListEntry entry;

    while (xml.readNextStartElement()) {
      if (xml.name() == u"id") {
        entry.anime_id = xml.readElementText().toInt();
      } else if (xml.name() == u"library_id") {
        entry.id = xml.readElementText().toStdString();
      } else if (xml.name() == u"progress") {
        entry.watched_episodes = xml.readElementText().toInt();
      } else if (xml.name() == u"date_start") {
        entry.date_started = FuzzyDate(xml.readElementText().toStdString());
      } else if (xml.name() == u"date_end") {
        entry.date_completed = FuzzyDate(xml.readElementText().toStdString());
      } else if (xml.name() == u"score") {
        entry.score = xml.readElementText().toInt();
      } else if (xml.name() == u"status") {
        entry.status = static_cast<anime::list::Status>(xml.readElementText().toInt());
      } else if (xml.name() == u"private") {
        entry.is_private = xml.readElementText().toInt();
      } else if (xml.name() == u"rewatched_times") {
        entry.rewatched_times = xml.readElementText().toInt();
      } else if (xml.name() == u"rewatching") {
        entry.rewatching = xml.readElementText().toInt();
      } else if (xml.name() == u"rewatching_ep") {
        entry.rewatching_ep = xml.readElementText().toInt();
      } else if (xml.name() == u"notes") {
        entry.notes = xml.readElementText().toStdString();
      } else if (xml.name() == u"last_updated") {
        entry.last_updated = xml.readElementText().toLongLong();
      } else {
        xml.skipCurrentElement();
      }
    }

    entries.emplace_back(entry);
  }

  return entries;
}

}  // namespace compat::v1
