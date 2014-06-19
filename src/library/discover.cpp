/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#include <algorithm>

#include "base/foreach.h"
#include "base/log.h"
#include "base/string.h"
#include "base/xml.h"
#include "library/anime_db.h"
#include "library/anime_item.h"
#include "library/anime_util.h"
#include "library/discover.h"
#include "sync/manager.h"
#include "sync/service.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"

library::SeasonDatabase SeasonDatabase;

namespace library {

bool SeasonDatabase::Load(std::wstring file) {
  items.clear();

  xml_document document;
  std::wstring path = taiga::GetPath(taiga::kPathDatabaseSeason) + file;
  xml_parse_result parse_result = document.load_file(path.c_str());

  if (parse_result.status != pugi::status_ok &&
      parse_result.status != pugi::status_file_not_found) {
    MessageBox(nullptr, L"Could not read season data.", path.c_str(),
               MB_OK | MB_ICONERROR);
    return false;
  }

  xml_node season_node = document.child(L"season");

  name = XmlReadStrValue(season_node.child(L"info"), L"name");
  time_t modified = _wtoi64(XmlReadStrValue(season_node.child(L"info"),
                                            L"modified").c_str());

  foreach_xmlnode_(node, season_node, L"anime") {
    std::map<enum_t, std::wstring> id_map;

    foreach_xmlnode_(id_node, node, L"id") {
      std::wstring id = id_node.child_value();
      std::wstring name = id_node.attribute(L"name").as_string();
      enum_t service_id = ServiceManager.GetServiceIdByName(name);
      id_map[service_id] = id;
    }

    int anime_id = anime::ID_UNKNOWN;
    anime::Item* anime_item = nullptr;

    foreach_(it, id_map) {
      anime_item = AnimeDatabase.FindItem(it->second, it->first);
      if (anime_item)
        break;
    }

    if (anime_item && anime_item->GetLastModified() >= modified) {
      anime_id = anime_item->GetId();
    } else {
      auto current_service_id = taiga::GetCurrentServiceId();
      if (id_map[current_service_id].empty()) {
        LOG(LevelDebug, name + L" - No ID for current service: " +
                        XmlReadStrValue(node, L"title"));
        continue;
      }

      anime::Item item;
      item.SetSource(sync::kMyAnimeList);
      foreach_(it, id_map)
        item.SetId(it->second, it->first);
      item.SetLastModified(modified);
      item.SetTitle(XmlReadStrValue(node, L"title"));
      item.SetType(XmlReadIntValue(node, L"type"));
      item.SetImageUrl(XmlReadStrValue(node, L"image"));
      item.SetProducers(XmlReadStrValue(node, L"producers"));
      anime_id = AnimeDatabase.UpdateItem(item);
    }

    items.push_back(anime_id);
  }

  return true;
}

bool SeasonDatabase::IsRefreshRequired() {
  int count = 0;
  bool required = false;

  foreach_(it, items) {
    int anime_id = *it;
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item) {
      const Date& date_start = anime_item->GetDateStart();
      if (!anime::IsValidDate(date_start) || anime_item->GetSynopsis().empty())
        count++;
    }
    if (count > 20) {
      required = true;
      break;
    }
  }

  return required;
}

void SeasonDatabase::Review(bool hide_nsfw) {
  Date date_start, date_end;
  anime::GetSeasonInterval(name, date_start, date_end);

  // Check for invalid items
  for (size_t i = 0; i < items.size(); i++) {
    int anime_id = items.at(i);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item) {
      bool invalid = false;
      // Airing date must be within the interval
      const Date& anime_start = anime_item->GetDateStart();
      if (anime::IsValidDate(anime_start))
        if (anime_start < date_start || anime_start > date_end)
          invalid = true;
      // Filter by age rating
      if (hide_nsfw && IsNsfw(*anime_item))
        invalid = true;
      if (invalid) {
        items.erase(items.begin() + i--);
        LOG(LevelDebug, L"Removed item: \"" + anime_item->GetTitle() +
                        L"\" (" + std::wstring(anime_start) + L")");
      }
    }
  }

  // Check for missing items
  foreach_(it, AnimeDatabase.items) {
    if (std::find(items.begin(), items.end(), it->second.GetId()) != items.end())
      continue;
    // Filter by age rating
    if (hide_nsfw && IsNsfw(it->second))
      continue;
    // Airing date must be within the interval
    const Date& anime_start = it->second.GetDateStart();
    if (anime_start.year && anime_start.month &&
        anime_start >= date_start && anime_start <= date_end) {
      items.push_back(it->second.GetId());
      LOG(LevelDebug, L"Added item: \"" + it->second.GetTitle() +
                      L"\" (" + std::wstring(anime_start) + L")");
    }
  }
}

}  // namespace library