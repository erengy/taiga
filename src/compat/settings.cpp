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

#include "settings.hpp"

#include <QFile>
#include <QXmlStreamReader>

namespace compat::v1 {

QMap<QString, QString> read_settings(const QString& path) {
  QFile file(path);

  if (!file.open(QIODevice::ReadOnly)) return {};

  QXmlStreamReader xml(&file);

  if (!xml.readNextStartElement()) return {};
  if (xml.name() != u"settings") return {};

  QString service;
  QString username;

  while (xml.readNextStartElement()) {
    if (xml.name() == u"account") {
      while (xml.readNextStartElement()) {
        if (xml.name() == u"update") {
          service = xml.attributes().value(u"activeservice").toString();
        } else if (xml.name() == service) {
          username = xml.attributes().value(u"username").toString();
        }
        xml.skipCurrentElement();
      }
    } else {
      xml.skipCurrentElement();
    }
  }

  return {
      {"service", service},
      {"username", username},
  };
}

}  // namespace compat::v1
