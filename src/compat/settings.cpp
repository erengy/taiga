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

#include <QXmlStreamReader>
#include <chrono>

#include "base/log.hpp"
#include "base/xml.hpp"
#include "taiga/accounts.hpp"
#include "taiga/settings.hpp"

#define XML_ATTR(name) xml.attributes().value(name)
#define XML_ATTR_BOOL(name) (XML_ATTR(name) == u"true")
#define XML_ATTR_INT(name) XML_ATTR(name).toInt()
#define XML_ATTR_STR(name) XML_ATTR(name).toString().toStdString()

namespace compat::v1 {

void parseAccountElement(QXmlStreamReader&, const taiga::Settings&, const taiga::Accounts&);
void parseAnimeElement(QXmlStreamReader&, const taiga::Settings&);
void parseRecognitionElement(QXmlStreamReader&, const taiga::Settings&);

void readSettings(const std::string& path, const taiga::Settings& settings,
                  const taiga::Accounts& accounts) {
  base::XmlFileReader xml;

  if (!xml.open(QString::fromStdString(path))) {
    LOGE("{}", xml.file().errorString().toStdString());
    return;
  }

  if (!xml.readElement(u"settings")) {
    xml.raiseError("Invalid settings file.");
  }

  while (xml.readNextStartElement()) {
    if (xml.name() == u"account") {
      parseAccountElement(xml, settings, accounts);
    } else if (xml.name() == u"anime") {
      parseAnimeElement(xml, settings);
    } else if (xml.name() == u"recognition") {
      parseRecognitionElement(xml, settings);
    } else {
      // @TODO: program, announce, rss
      xml.skipCurrentElement();
    }
  }

  if (xml.hasError()) {
    LOGE("{}", xml.errorString().toStdString());
  }
}

void parseAccountElement(QXmlStreamReader& xml, const taiga::Settings& settings,
                         const taiga::Accounts& accounts) {
  while (xml.readNextStartElement()) {
    if (xml.name() == u"update") {
      settings.setService(XML_ATTR_STR(u"activeservice"));
      xml.skipCurrentElement();

    } else if (xml.name() == u"anilist") {
      accounts.setAnilistUsername(XML_ATTR_STR(u"username"));
      accounts.setAnilistToken(XML_ATTR_STR(u"token"));
      xml.skipCurrentElement();

    } else if (xml.name() == u"kitsu") {
      accounts.setKitsuEmail(XML_ATTR_STR(u"email"));
      accounts.setKitsuUsername(XML_ATTR_STR(u"username"));
      accounts.setKitsuPassword(XML_ATTR_STR(u"password"));
      xml.skipCurrentElement();

    } else if (xml.name() == u"myanimelist") {
      accounts.setMyanimelistUsername(XML_ATTR_STR(u"username"));
      accounts.setMyanimelistAccessToken(XML_ATTR_STR(u"accesstoken"));
      accounts.setMyanimelistRefreshToken(XML_ATTR_STR(u"refreshtoken"));
      xml.skipCurrentElement();

    } else {
      xml.skipCurrentElement();
    }
  }
}

void parseAnimeElement(QXmlStreamReader& xml, const taiga::Settings& settings) {
  std::vector<std::string> libraryFolders;

  while (xml.readNextStartElement()) {
    if (xml.name() == u"folders") {
      while (xml.readNextStartElement()) {
        if (xml.name() == u"root") {
          libraryFolders.push_back(XML_ATTR_STR(u"folder"));
          xml.skipCurrentElement();
        } else {
          xml.skipCurrentElement();
        }
      }

    } else if (xml.name() == u"items") {
      while (xml.readNextStartElement()) {
        if (xml.name() == u"item") {
          const int id = XML_ATTR_INT(u"id");
          const auto folder = XML_ATTR_STR(u"folder");
          const auto title = XML_ATTR_STR(u"title");
          // @TODO: Store values
          xml.skipCurrentElement();
        } else {
          xml.skipCurrentElement();
        }
      }

    } else {
      xml.skipCurrentElement();
    }
  }

  settings.setLibraryFolders(libraryFolders);
}

void parseRecognitionElement(QXmlStreamReader& xml, const taiga::Settings& settings) {
  while (xml.readNextStartElement()) {
    if (xml.name() == u"mediaplayers") {
      std::vector<std::string> disabledPlayers;
      while (xml.readNextStartElement()) {
        if (xml.name() == u"player") {
          if (!XML_ATTR_BOOL(u"enabled")) {
            disabledPlayers.push_back(XML_ATTR_STR(u"name"));
          }
          xml.skipCurrentElement();
        } else {
          xml.skipCurrentElement();
        }
      }
      settings.setDisabledMediaPlayers(disabledPlayers);
      xml.skipCurrentElement();

    } else if (xml.name() == u"general") {
      const auto seconds = std::chrono::seconds{XML_ATTR_INT(u"detectioninterval")};
      settings.setMediaDetectionInterval(seconds);
      xml.skipCurrentElement();

    } else {
      xml.skipCurrentElement();
    }
  }
}

}  // namespace compat::v1
