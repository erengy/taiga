/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#include "std.h"
#include <algorithm>
#include "anime.h"
#include "animedb.h"
#include "animelist.h"
#include "common.h"
#include "myanimelist.h"
#include "string.h"
#include "taiga.h"
#include "xml.h"

class AnimeDatabase AnimeDatabase;
class AnimeSeasonDatabase SeasonDatabase;

// =============================================================================

AnimeDatabase::AnimeDatabase() {
  folder_ = Taiga.GetDataPath() + L"db\\";
  file_ = L"anime.xml";
}

bool AnimeDatabase::Load() {
  // Initialize
  wstring file = folder_ + file_;
  items.clear();
  
  // Load XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != status_ok && result.status != status_file_not_found) {
    return false;
  }

  // Read items
  size_t anime_count = 0, i = 0;
  xml_node animedb_node = doc.child(L"animedb");
  for (xml_node node = animedb_node.child(L"anime"); node; node = node.next_sibling(L"anime")) {
    anime_count++;
  }
  if (anime_count > 0) {
    items.resize(anime_count);
  }
  for (xml_node node = animedb_node.child(L"anime"); node; node = node.next_sibling(L"anime")) {
    items.at(i).index = -1;
    
    items.at(i).series_id = XML_ReadIntValue(node, L"series_animedb_id");
    items.at(i).series_title = XML_ReadStrValue(node, L"series_title");
    items.at(i).series_synonyms = XML_ReadStrValue(node, L"series_synonyms");
    items.at(i).series_type = XML_ReadIntValue(node, L"series_type");
    items.at(i).series_episodes = XML_ReadIntValue(node, L"series_episodes");
    items.at(i).series_status = XML_ReadIntValue(node, L"series_status");
    items.at(i).series_start = XML_ReadStrValue(node, L"series_start");
    items.at(i).series_end = XML_ReadStrValue(node, L"series_end");
    items.at(i).series_image = XML_ReadStrValue(node, L"series_image");
    
    items.at(i).genres = XML_ReadStrValue(node, L"genres");
    items.at(i).producers = XML_ReadStrValue(node, L"producers");
    items.at(i).score = XML_ReadStrValue(node, L"score");
    items.at(i).rank = XML_ReadStrValue(node, L"rank");
    items.at(i).popularity = XML_ReadStrValue(node, L"popularity");
    items.at(i).synopsis = XML_ReadStrValue(node, L"synopsis");
    
    items.at(i).last_modified = _wtoi64(XML_ReadStrValue(node, L"last_modified").c_str());
    i++;
  }

  return true;
}

bool AnimeDatabase::Save() {
  for (size_t i = 1; i < AnimeList.items.size(); i++) {
    Update(AnimeList.items.at(i));
  }

  if (items.empty()) return false;

  // Initialize
  xml_document doc;
  xml_node animedb_node = doc.append_child();
  animedb_node.set_name(L"animedb");

  // Sort items
  std::sort(items.begin(), items.end(), 
    [](const Anime& a1, const Anime& a2) {
      return a1.series_id < a2.series_id;
    });

  // Write items
  for (auto it = items.begin(); it != items.end(); ++it) {
    xml_node anime_node = animedb_node.append_child();
    anime_node.set_name(L"anime");

    XML_WriteIntValue(anime_node, L"series_animedb_id", it->series_id);
    XML_WriteStrValue(anime_node, L"series_title", it->series_title.c_str(), node_cdata);
    if (!it->series_synonyms.empty())
      XML_WriteStrValue(anime_node, L"series_synonyms", it->series_synonyms.c_str(), node_cdata);
    XML_WriteIntValue(anime_node, L"series_type", it->series_type);
    XML_WriteIntValue(anime_node, L"series_episodes", it->series_episodes);
    XML_WriteIntValue(anime_node, L"series_status", it->series_status);
    XML_WriteStrValue(anime_node, L"series_start", it->series_start.c_str());
    XML_WriteStrValue(anime_node, L"series_end", it->series_end.c_str());
    XML_WriteStrValue(anime_node, L"series_image", it->series_image.c_str());
    
    if (!it->genres.empty())
      XML_WriteStrValue(anime_node, L"genres", it->genres.c_str());
    if (!it->producers.empty())
      XML_WriteStrValue(anime_node, L"producers", it->producers.c_str());
    if (!it->score.empty())
      XML_WriteStrValue(anime_node, L"score", it->score.c_str());
    if (!it->rank.empty())
      XML_WriteStrValue(anime_node, L"rank", it->rank.c_str());
    if (!it->popularity.empty())
      XML_WriteStrValue(anime_node, L"popularity", it->popularity.c_str());
    if (!it->synopsis.empty())
      XML_WriteStrValue(anime_node, L"synopsis", it->synopsis.c_str(), node_cdata);
    
    XML_WriteStrValue(anime_node, L"last_modified", ToWSTR(it->last_modified).c_str());
  }

  // Save
  CreateFolder(folder_);
  wstring file = folder_ + file_;
  return doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
}

Anime* AnimeDatabase::FindItem(int anime_id) {
  if (anime_id)
    for (size_t i = 0; i < items.size(); i++)
      if (items.at(i).series_id == anime_id)
        return &items.at(i);
  return nullptr;
}

void AnimeDatabase::Update(Anime& anime) {
  critical_section_.Enter();
  
  Anime* item = FindItem(anime.series_id);
  if (item == nullptr) {
    items.resize(items.size() + 1);
    item = &items.back();
  }
  item->Update(anime);

  critical_section_.Leave();
}

// =============================================================================

AnimeSeasonDatabase::AnimeSeasonDatabase() : 
  last_modified(0), modified(false)
{
  folder_ = Taiga.GetDataPath() + L"db\\season\\";
}

bool AnimeSeasonDatabase::Load(wstring file) {
  // Write modifications
  if (modified && !file.empty()) Save();

  // Initialize
  file_ = file;
  file = folder_ + file;
  items.clear();
  
  // Load XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != status_ok && result.status != status_file_not_found) {
    MessageBox(NULL, L"Could not read season data.", file.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  // Read information
  xml_node season_node = doc.child(L"season");
  name = XML_ReadStrValue(season_node.child(L"info"), L"name");
  last_modified = _wtoi64(XML_ReadStrValue(season_node.child(L"info"), L"last_modified").c_str());

  // Read items
  size_t anime_count = 0, i = 0;
  for (xml_node node = season_node.child(L"anime"); node; node = node.next_sibling(L"anime")) {
    anime_count++;
  }
  if (anime_count > 0) {
    items.resize(anime_count);
  }
  for (xml_node node = season_node.child(L"anime"); node; node = node.next_sibling(L"anime")) {
    items.at(i).index = -1;
    items.at(i).series_id = XML_ReadIntValue(node, L"series_animedb_id");
    items.at(i).series_title = XML_ReadStrValue(node, L"series_title");
    items.at(i).series_type = XML_ReadIntValue(node, L"series_type");
    items.at(i).series_image = XML_ReadStrValue(node, L"series_image");
    items.at(i).producers = XML_ReadStrValue(node, L"producers");

    xml_node settings_node = node.child(L"settings");
    items.at(i).settings_keep_title = XML_ReadIntValue(settings_node, L"keep_title");

    Anime* anime = AnimeDatabase.FindItem(items.at(i).series_id);
    if (anime != nullptr) {
      items.at(i).Update(*anime);
      if (anime->last_modified > this->last_modified) {
        this->last_modified = anime->last_modified;
      }
    }

    i++;
  }

  return true;
}

bool AnimeSeasonDatabase::Save(wstring file) {
  if (!modified) return false;
  if (items.empty()) return false;
  if (file_.empty() && file.empty()) return false;

  for (auto it = items.begin(); it != items.end(); ++it) {
    AnimeDatabase.Update(*it);
  }

#ifdef _DEBUG
  if (file.empty()) file = file_;
  file = folder_ + file;
  modified = false;

  std::sort(items.begin(), items.end(), 
    [](const Anime& a1, const Anime& a2) {
      return CompareStrings(a1.series_title, a2.series_title) < 0;
    });

  xml_document doc;
  xml_node season_node = doc.append_child();
  season_node.set_name(L"season");

  xml_node info_node = season_node.append_child();
  info_node.set_name(L"info");
  XML_WriteStrValue(info_node, L"name", name.c_str());
  XML_WriteStrValue(info_node, L"last_modified", ToWSTR(last_modified).c_str());

  for (auto it = items.begin(); it != items.end(); ++it) {
    xml_node anime_node = season_node.append_child();
    anime_node.set_name(L"anime");
    XML_WriteIntValue(anime_node, L"series_animedb_id", it->series_id);
    XML_WriteStrValue(anime_node, L"series_title", it->series_title.c_str(), node_cdata);
    XML_WriteIntValue(anime_node, L"series_type", it->series_type);
    XML_WriteStrValue(anime_node, L"series_image", it->series_image.c_str());
    XML_WriteStrValue(anime_node, L"producers", it->producers.c_str());
    if (it->settings_keep_title) {
      xml_node settings_node = anime_node.append_child();
      settings_node.set_name(L"settings");
      XML_WriteIntValue(settings_node, L"keep_title", it->settings_keep_title);
    }
  }

  CreateFolder(folder_);
  return doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);

#else
  return true;
#endif
}