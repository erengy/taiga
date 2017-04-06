/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "base/foreach.h"
#include "base/log.h"
#include "base/string.h"
#include "base/xml.h"
#include "library/anime_db.h"
#include "library/history.h"
#include "sync/sync.h"

void History::ReadQueueInCompatibilityMode(const pugi::xml_document& document) {
  xml_node node_queue = document.child(L"history").child(L"queue");

  foreach_xmlnode_(item, node_queue, L"item") {
    HistoryItem history_item;

    history_item.anime_id = item.attribute(L"anime_id").as_int(anime::ID_NOTINLIST);
    history_item.mode = item.attribute(L"mode").as_int();
    history_item.time = item.attribute(L"time").value();

    #define READ_ATTRIBUTE_INT(x, y) \
        if (!item.attribute(y).empty()) x = item.attribute(y).as_int();
    #define READ_ATTRIBUTE_STR(x, y) \
        if (!item.attribute(y).empty()) x = item.attribute(y).as_string();
    #define READ_ATTRIBUTE_DATE(x, y) \
        if (!item.attribute(y).empty()) x = (Date)item.attribute(y).as_string();

    READ_ATTRIBUTE_INT(history_item.episode, L"episode");
    READ_ATTRIBUTE_INT(history_item.score, L"score");
    READ_ATTRIBUTE_INT(history_item.status, L"status");
    READ_ATTRIBUTE_INT(history_item.enable_rewatching, L"enable_rewatching");
    READ_ATTRIBUTE_STR(history_item.tags, L"tags");
    READ_ATTRIBUTE_DATE(history_item.date_start, L"date_start");
    READ_ATTRIBUTE_DATE(history_item.date_finish, L"date_finish");

    #undef READ_ATTRIBUTE_DATE
    #undef READ_ATTRIBUTE_STR
    #undef READ_ATTRIBUTE_INT

    Date date_item(history_item.time);
    Date date_limit(L"2014-06-20");  // Release date of v1.1.0
    if (date_item < date_limit) {
      if (history_item.mode == 3) {         // HTTP_MAL_AnimeAdd
        history_item.mode = taiga::kHttpServiceAddLibraryEntry;
      } else if (history_item.mode == 5) {  // HTTP_MAL_AnimeDelete
        history_item.mode = taiga::kHttpServiceDeleteLibraryEntry;
      } else if (history_item.mode == 7) {  // HTTP_MAL_AnimeUpdate
        history_item.mode = taiga::kHttpServiceUpdateLibraryEntry;
      }
    }

    if (AnimeDatabase.FindItem(history_item.anime_id)) {
      queue.Add(history_item, false);
    } else {
      LOGW(L"Item does not exist in the database.\n"
           L"ID: " + ToWstr(history_item.anime_id));
    }
  }
}
