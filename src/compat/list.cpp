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

#include "list.hpp"

#include <QXmlStreamReader>

#include "base/log.hpp"
#include "base/xml.hpp"
#include "compat/common.hpp"

#define XML_ELEMENT xml.readElementText()

namespace compat::v1 {

ListEntry parseListEntryElement(QXmlStreamReader& xml);

QList<ListEntry> readListEntries(const std::string& path) {
  base::XmlFileReader xml;

  if (!xml.open(QString::fromStdString(path), removeMetaElement)) {
    qCritical() << xml.file().errorString();
    return {};
  }

  if (!xml.readElement(u"library")) {
    xml.raiseError("Invalid anime list file.");
  }

  QList<ListEntry> entries;

  while (xml.readElement(u"anime")) {
    entries.emplace_back(parseListEntryElement(xml));
  }

  if (xml.hasError()) {
    qCritical() << xml.errorString();
  }

  return entries;
}

ListEntry parseListEntryElement(QXmlStreamReader& xml) {
  ListEntry entry;

  while (xml.readNextStartElement()) {
    if (xml.name() == u"id") {
      entry.anime_id = XML_ELEMENT.toInt();

    } else if (xml.name() == u"library_id") {
      entry.id = XML_ELEMENT.toLongLong();

    } else if (xml.name() == u"progress") {
      entry.watched_episodes = XML_ELEMENT.toInt();

    } else if (xml.name() == u"date_start") {
      entry.date_started = FuzzyDate(XML_ELEMENT.toStdString());

    } else if (xml.name() == u"date_end") {
      entry.date_completed = FuzzyDate(XML_ELEMENT.toStdString());

    } else if (xml.name() == u"score") {
      entry.score = XML_ELEMENT.toInt();

    } else if (xml.name() == u"status") {
      entry.status = static_cast<anime::list::Status>(XML_ELEMENT.toInt());

    } else if (xml.name() == u"private") {
      entry.is_private = XML_ELEMENT.toInt();

    } else if (xml.name() == u"rewatched_times") {
      entry.rewatched_times = XML_ELEMENT.toInt();

    } else if (xml.name() == u"rewatching") {
      entry.rewatching = XML_ELEMENT.toInt();

    } else if (xml.name() == u"rewatching_ep") {
      entry.rewatching_ep = XML_ELEMENT.toInt();

    } else if (xml.name() == u"notes") {
      entry.notes = XML_ELEMENT.toStdString();

    } else if (xml.name() == u"last_updated") {
      entry.last_updated = XML_ELEMENT.toLongLong();

    } else {
      xml.skipCurrentElement();
    }
  }

  return entry;
}

}  // namespace compat::v1
