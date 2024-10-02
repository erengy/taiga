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

#include <QFile>
#include <QXmlStreamReader>

#include "anime_db.hpp"

namespace anime {

void Database::read() {
  data_.clear();

  QFile file("./data/anime.xml");

  if (!file.open(QIODevice::ReadOnly)) return;

  QXmlStreamReader xml(&file);

  if (!xml.readNextStartElement()) return;
  if (xml.name() != u"database") return;

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
        anime.type = static_cast<Type>(xml.readElementText().toInt());
      } else if (xml.name() == u"status") {
        anime.status = static_cast<Status>(xml.readElementText().toInt());
      } else if (xml.name() == u"episode_count") {
        anime.episode_count = xml.readElementText().toInt();
      } else if (xml.name() == u"episode_length") {
        anime.episode_length = xml.readElementText().toInt();
      } else if (xml.name() == u"date_start") {
        anime.start_date = FuzzyDate(xml.readElementText().toStdString());
      } else if (xml.name() == u"date_end") {
        anime.end_date = FuzzyDate(xml.readElementText().toStdString());
      } else if (xml.name() == u"image") {
        anime.image_url = xml.readElementText().toStdString();
      } else if (xml.name() == u"trailer_id") {
        anime.trailer_id = xml.readElementText().toStdString();
      } else if (xml.name() == u"age_rating") {
        anime.age_rating = static_cast<AgeRating>(xml.readElementText().toInt());
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

    data_.emplace_back(anime);
  }
}

}  // namespace anime
