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

#include "history.hpp"

#include <QDateTime>

#include "base/log.hpp"
#include "base/string.hpp"
#include "base/xml.hpp"
#include "compat/common.hpp"
#include "media/anime_history.hpp"

#define XML_ATTR(name) xml.attributes().value(name)
#define XML_ATTR_BOOL(name) (XML_ATTR(name) == u"true")

namespace compat::v1 {

void parseItems(QXmlStreamReader& xml, QList<anime::HistoryItem>& items);
void parseQueue(QXmlStreamReader& xml, QList<QueueItem>& items);

QList<anime::HistoryItem> readHistory(const std::string& path) {
  base::XmlFileReader xml;

  if (!xml.open(QString::fromStdString(path), removeMetaElement)) {
    qCritical() << xml.file().errorString();
    return {};
  }

  if (!xml.readElement(u"history")) {
    xml.raiseError("Invalid history file.");
  }

  QList<anime::HistoryItem> items;

  while (xml.readNextStartElement()) {
    if (xml.name() == u"items") {
      parseItems(xml, items);
    } else {
      xml.skipCurrentElement();
    }
  }

  if (xml.hasError()) {
    qCritical() << xml.errorString();
    return {};
  }

  return items;
}

QList<QueueItem> readQueue(const std::string& path) {
  base::XmlFileReader xml;

  if (!xml.open(QString::fromStdString(path), removeMetaElement)) {
    qCritical() << xml.file().errorString();
    return {};
  }

  if (!xml.readElement(u"history")) {
    xml.raiseError("Invalid history file.");
  }

  QList<QueueItem> items;

  while (xml.readNextStartElement()) {
    if (xml.name() == u"queue") {
      parseQueue(xml, items);
    } else {
      xml.skipCurrentElement();
    }
  }

  if (xml.hasError()) {
    qCritical() << xml.errorString();
    return {};
  }

  return items;
}

void parseItems(QXmlStreamReader& xml, QList<anime::HistoryItem>& items) {
  const auto parseTime = [](const QString& value) {
    const auto datetime = QDateTime::fromString(value, u"yyyy-MM-dd HH:mm:ss"_s);
    return datetime.isValid() ? static_cast<std::time_t>(datetime.toSecsSinceEpoch()) : 0;
  };

  while (xml.readNextStartElement()) {
    if (xml.name() == u"item") {
      items.emplace_back(anime::HistoryItem{
          .anime_id = XML_ATTR(u"anime_id").toInt(),
          .episode = XML_ATTR(u"episode").toInt(),
          .time = parseTime(XML_ATTR(u"time").toString()),
      });
      xml.skipCurrentElement();

    } else {
      xml.skipCurrentElement();
    }
  }
}

void parseQueue(QXmlStreamReader& xml, QList<QueueItem>& items) {
  while (xml.readNextStartElement()) {
    if (xml.name() == u"item") {
      const auto attrs = xml.attributes();

      QueueItem item{
          .anime_id = XML_ATTR(u"anime_id").toInt(),
          .delete_entry = XML_ATTR(u"mode") == u"delete",
      };

      if (attrs.hasAttribute(u"episode")) item.episode = XML_ATTR(u"episode").toInt();
      if (attrs.hasAttribute(u"score")) item.score = XML_ATTR(u"score").toInt();
      if (attrs.hasAttribute(u"status")) {
        item.status = static_cast<anime::list::Status>(XML_ATTR(u"status").toInt());
      }
      if (attrs.hasAttribute(u"enable_rewatching")) {
        item.rewatching = XML_ATTR_BOOL(u"enable_rewatching");
      }
      if (attrs.hasAttribute(u"rewatched_times")) {
        item.rewatched_times = XML_ATTR(u"rewatched_times").toInt();
      }
      if (attrs.hasAttribute(u"notes")) {
        item.notes = XML_ATTR(u"notes").toString().toStdString();
      }
      if (attrs.hasAttribute(u"date_start")) {
        item.date_started = FuzzyDate(XML_ATTR(u"date_start").toString().toStdString());
      }
      if (attrs.hasAttribute(u"date_finish")) {
        item.date_completed = FuzzyDate(XML_ATTR(u"date_finish").toString().toStdString());
      }

      items.emplace_back(item);
      xml.skipCurrentElement();

    } else {
      xml.skipCurrentElement();
    }
  }
}

}  // namespace compat::v1
