/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

#include "anime_db.h"
#include "anime_item.h"
#include "anime_util.h"
#include "discover.h"
#include "base/logger.h"
#include "base/string.h"
#include "base/xml.h"
#include "taiga/taiga.h"

library::SeasonDatabase SeasonDatabase;

namespace library {

SeasonDatabase::SeasonDatabase() {
  folder_ = Taiga.GetDataPath() + L"db\\season\\";
}

bool SeasonDatabase::Load(wstring file) {
  // Initialize
  file_ = file;
  file = folder_ + file;
  items.clear();
  
  // Load XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != pugi::status_ok && result.status != pugi::status_file_not_found) {
    MessageBox(NULL, L"Could not read season data.", file.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  // Read information
  xml_node season_node = doc.child(L"season");
  name = XmlReadStrValue(season_node.child(L"info"), L"name");
  time_t last_modified = _wtoi64(XmlReadStrValue(season_node.child(L"info"), L"last_modified").c_str());

  // Read items
  for (xml_node node = season_node.child(L"anime"); node; node = node.next_sibling(L"anime")) {
    int anime_id = XmlReadIntValue(node, L"series_animedb_id");
    items.push_back(anime_id);
    
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item && anime_item->last_modified >= last_modified)
      continue;

    anime::Item item;
    item.SetId(XmlReadIntValue(node, L"series_animedb_id"));
    item.SetTitle(XmlReadStrValue(node, L"series_title"));
    item.SetType(XmlReadIntValue(node, L"series_type"));
    item.SetImageUrl(XmlReadStrValue(node, L"series_image"));
    item.SetProducers(XmlReadStrValue(node, L"producers"));
    xml_node settings_node = node.child(L"settings");
    item.keep_title = XmlReadIntValue(settings_node, L"keep_title") != 0;
    item.last_modified = last_modified;
    AnimeDatabase.UpdateItem(item);
  }

  return true;
}

bool SeasonDatabase::IsRefreshRequired() {
  int count = 0;
  bool required = false;

  for (size_t i = 0; i < items.size(); i++) {
    int anime_id = items.at(i);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item) {
      const Date& date_start = anime_item->GetDate(anime::DATE_START);
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

void SeasonDatabase::Review(bool hide_hentai) {
  Date date_start, date_end;
  anime::GetSeasonInterval(name, date_start, date_end);

  // Check for invalid items
  for (size_t i = 0; i < items.size(); i++) {
    int anime_id = items.at(i);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item) {
      bool invalid = false;
      // Airing date must be within the interval
      const Date& anime_start = anime_item->GetDate(anime::DATE_START);
      if (anime::IsValidDate(anime_start))
        if (anime_start < date_start || anime_start > date_end)
          invalid = true;
      // TODO: Filter by rating instead if made possible in API
      wstring genres = Join(anime_item->GetGenres(), L", ");
      if (hide_hentai && InStr(genres, L"Hentai", 0, true) > -1)
        invalid = true;
      if (invalid) {
        items.erase(items.begin() + i--);
        LOG(LevelDebug, L"Removed item: \"" + anime_item->GetTitle() +
                        L"\" (" + wstring(anime_start) + L")");
      }
    }
  }

  // Check for missing items
  for (auto it = AnimeDatabase.items.begin(); it != AnimeDatabase.items.end(); ++it) {
    if (std::find(items.begin(), items.end(), it->second.GetId()) != items.end())
      continue;
    // TODO: Filter by rating instead if made possible in API
    wstring genres = Join(it->second.GetGenres(), L", ");
    if (hide_hentai && InStr(genres, L"Hentai", 0, true) > -1)
      continue;
    // Airing date must be within the interval
    const Date& anime_start = it->second.GetDate(anime::DATE_START);
    if (anime_start.year && anime_start.month &&
        anime_start >= date_start && anime_start <= date_end) {
      items.push_back(it->second.GetId());
      LOG(LevelDebug, L"Added item: \"" + it->second.GetTitle() +
                      L"\" (" + wstring(anime_start) + L")");
    }
  }
}

}  // namespace library