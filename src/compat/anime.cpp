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

#include "anime.hpp"

#include <QXmlStreamReader>

#include "base/log.hpp"
#include "base/string.hpp"
#include "base/xml.hpp"
#include "compat/common.hpp"

#define XML_ELEMENT xml.readElementText()

namespace compat::v1 {

Anime parseAnimeElement(QXmlStreamReader& xml);

QList<Anime> readAnimeDatabase(const std::string& path) {
  base::XmlFileReader xml;

  if (!xml.open(QString::fromStdString(path), removeMetaElement)) {
    qCritical() << xml.file().errorString();
    return {};
  }

  if (!xml.readElement(u"database")) {
    xml.raiseError("Invalid anime database file.");
  }

  QList<Anime> db;

  while (xml.readElement(u"anime")) {
    db.emplace_back(parseAnimeElement(xml));
  }

  if (xml.hasError()) {
    qCritical() << xml.errorString();
  }

  return db;
}

Anime parseAnimeElement(QXmlStreamReader& xml) {
  Anime anime;

  while (xml.readNextStartElement()) {
    if (xml.name() == u"id") {
      // @TODO: Store ID from current `source`
      anime.id = XML_ELEMENT.toInt();

    } else if (xml.name() == u"slug") {
      anime.slug = XML_ELEMENT.toStdString();

    } else if (xml.name() == u"title") {
      anime.titles.romaji = XML_ELEMENT.toStdString();

    } else if (xml.name() == u"english") {
      anime.titles.english = XML_ELEMENT.toStdString();

    } else if (xml.name() == u"japanese") {
      anime.titles.japanese = XML_ELEMENT.toStdString();

    } else if (xml.name() == u"synonym") {
      anime.titles.synonyms.push_back(XML_ELEMENT.toStdString());

    } else if (xml.name() == u"type") {
      anime.type = static_cast<anime::Type>(XML_ELEMENT.toInt());

    } else if (xml.name() == u"status") {
      anime.status = static_cast<anime::Status>(XML_ELEMENT.toInt());

    } else if (xml.name() == u"episode_count") {
      anime.episodeCount = XML_ELEMENT.toInt();

    } else if (xml.name() == u"episode_length") {
      anime.episodeLength = XML_ELEMENT.toInt();

    } else if (xml.name() == u"date_start") {
      anime.dateStarted = FuzzyDate(XML_ELEMENT.toStdString());

    } else if (xml.name() == u"date_end") {
      anime.dateFinished = FuzzyDate(XML_ELEMENT.toStdString());

    } else if (xml.name() == u"image") {
      anime.imageUrl = XML_ELEMENT.toStdString();

    } else if (xml.name() == u"trailer_id") {
      anime.trailerId = XML_ELEMENT.toStdString();

    } else if (xml.name() == u"age_rating") {
      anime.ageRating = static_cast<anime::AgeRating>(XML_ELEMENT.toInt());

    } else if (xml.name() == u"genres") {
      anime.genres = toVector(XML_ELEMENT.split(", ", Qt::SkipEmptyParts));

    } else if (xml.name() == u"tags") {
      anime.tags = toVector(XML_ELEMENT.split(", ", Qt::SkipEmptyParts));

    } else if (xml.name() == u"producers") {
      anime.producers = toVector(XML_ELEMENT.split(", ", Qt::SkipEmptyParts));

    } else if (xml.name() == u"studios") {
      anime.studios = toVector(XML_ELEMENT.split(", ", Qt::SkipEmptyParts));

    } else if (xml.name() == u"score") {
      anime.score = XML_ELEMENT.toFloat();

    } else if (xml.name() == u"popularity") {
      anime.popularityRank = XML_ELEMENT.toInt();

    } else if (xml.name() == u"synopsis") {
      anime.synopsis = XML_ELEMENT.toStdString();

    } else if (xml.name() == u"last_aired_episode") {
      anime.lastAiredEpisode = XML_ELEMENT.toInt();

    } else if (xml.name() == u"next_episode_time") {
      anime.nextEpisodeTime = XML_ELEMENT.toInt();

    } else if (xml.name() == u"modified") {
      anime.lastModified = XML_ELEMENT.toInt();

    } else {
      xml.skipCurrentElement();
    }
  }

  return anime;
}

}  // namespace compat::v1
