/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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
#include "common.h"
#include "myanimelist.h"
#include "string.h"
#include "taiga.h"
#include "xml.h"

AnimeSeasonDatabase SeasonDatabase;

// =============================================================================

AnimeSeasonDatabase::AnimeSeasonDatabase() : 
  modified(false)
{
}

bool AnimeSeasonDatabase::Load(wstring file) {
  // Write modifications
  if (modified && !file.empty()) Save();

  // Initialize
  folder_ = Taiga.GetDataPath() + L"db\\season\\";
  file_ = file; file = folder_ + file;
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
  last_modified = XML_ReadStrValue(season_node.child(L"info"), L"last_modified");

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
    xml_node settings_node = node.child(L"settings");
    items.at(i).settings_keep_title = XML_ReadIntValue(settings_node, L"keep_title");
    MAL.DecodeText(items.at(i).series_title);
    MAL.DecodeText(items.at(i).series_synonyms);
    MAL.DecodeText(items.at(i).synopsis);
    i++;
  }

  return true;
}

bool AnimeSeasonDatabase::Save(wstring file, bool minimal) {
  if (items.empty()) return false;
  if (!modified && !minimal) return false;
  if (file_.empty() && file.empty()) return false;

  if (file.empty()) file = file_;
  if (minimal) file = L"_" + file;
  file = folder_ + file;
  if (!minimal) modified = false;

  if (minimal) {
    std::sort(items.begin(), items.end(), 
      [](const Anime& a1, const Anime& a2) {
        return CompareStrings(a1.series_title, a2.series_title) < 0;
      });
  }

  xml_document doc;
  xml_node season_node = doc.append_child();
  season_node.set_name(L"season");

  // Info
  xml_node info_node = season_node.append_child();
  info_node.set_name(L"info");
  XML_WriteStrValue(info_node, L"name", name.c_str());
  XML_WriteStrValue(info_node, L"last_modified", last_modified.c_str());

  // Items
  for (auto it = items.begin(); it != items.end(); ++it) {
    xml_node anime_node = season_node.append_child();
    anime_node.set_name(L"anime");
    if (minimal) {
      XML_WriteIntValue(anime_node, L"series_animedb_id", it->series_id);
      XML_WriteStrValue(anime_node, L"series_title", it->series_title.c_str(), node_cdata);
      XML_WriteIntValue(anime_node, L"series_type", it->series_type);
      XML_WriteStrValue(anime_node, L"series_image", it->series_image.c_str());
      XML_WriteStrValue(anime_node, L"producers", it->producers.c_str());
    } else {
      XML_WriteIntValue(anime_node, L"series_animedb_id", it->series_id);
      XML_WriteStrValue(anime_node, L"series_title", it->series_title.c_str(), node_cdata);
      XML_WriteStrValue(anime_node, L"series_synonyms", it->series_synonyms.c_str(), node_cdata);
      XML_WriteIntValue(anime_node, L"series_type", it->series_type);
      XML_WriteIntValue(anime_node, L"series_episodes", it->series_episodes);
      XML_WriteIntValue(anime_node, L"series_status", it->series_status);
      XML_WriteStrValue(anime_node, L"series_start", it->series_start.c_str());
      XML_WriteStrValue(anime_node, L"series_end", it->series_end.c_str());
      XML_WriteStrValue(anime_node, L"series_image", it->series_image.c_str());
      XML_WriteStrValue(anime_node, L"genres", it->genres.c_str());
      XML_WriteStrValue(anime_node, L"producers", it->producers.c_str());
      XML_WriteStrValue(anime_node, L"score", it->score.c_str());
      XML_WriteStrValue(anime_node, L"rank", it->rank.c_str());
      XML_WriteStrValue(anime_node, L"popularity", it->popularity.c_str());
      XML_WriteStrValue(anime_node, L"synopsis", it->synopsis.c_str());
    }
    if (it->settings_keep_title) {
      xml_node settings_node = anime_node.append_child();
      settings_node.set_name(L"settings");
      XML_WriteIntValue(settings_node, L"keep_title", it->settings_keep_title);
    }
  }

  CreateFolder(folder_);
  return doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
}