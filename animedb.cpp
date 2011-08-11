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
#include "animedb.h"
#include "myanimelist.h"
#include "taiga.h"
#include "xml.h"

CAnimeDatabase AnimeDatabase;
CAnimeSeasonDatabase SeasonDatabase;

// =============================================================================

bool CAnimeSeasonDatabase::Read(wstring file) {
  Items.clear();
  file = Taiga.GetDataPath() + L"db\\season\\" + file;
  
  // Read XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != status_ok && result.status != status_file_not_found) {
    MessageBox(NULL, L"Could not read season data.", file.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  // Read information
  xml_node season = doc.child(L"season");
  Name = XML_ReadStrValue(season.child(L"info"), L"name");

  // Read items
  for (xml_node anime = season.child(L"anime"); anime; anime = anime.next_sibling(L"anime")) {
    Items.resize(Items.size() + 1);
    Items.back().Index = -1;
    Items.back().Series_ID = XML_ReadIntValue(anime, L"series_animedb_id");
    Items.back().Series_Title = XML_ReadStrValue(anime, L"series_title");
    Items.back().Series_Synonyms = XML_ReadStrValue(anime, L"series_synonyms");
    Items.back().Series_Type = XML_ReadIntValue(anime, L"series_type");
    Items.back().Series_Episodes = XML_ReadIntValue(anime, L"series_episodes");
    Items.back().Series_Status = XML_ReadIntValue(anime, L"series_status");
    Items.back().Series_Start = XML_ReadStrValue(anime, L"series_start");
    Items.back().Series_End = XML_ReadStrValue(anime, L"series_end");
    Items.back().Series_Image = XML_ReadStrValue(anime, L"series_image");
    Items.back().Genres = XML_ReadStrValue(anime, L"genres");
    Items.back().Producers = XML_ReadStrValue(anime, L"producers");
    Items.back().Score = XML_ReadStrValue(anime, L"score");
    Items.back().Rank = XML_ReadStrValue(anime, L"rank");
    Items.back().Popularity = XML_ReadStrValue(anime, L"popularity");
    Items.back().Synopsis = XML_ReadStrValue(anime, L"synopsis");
    MAL.DecodeText(Items.back().Synopsis);
  }

  return true;
}

bool CAnimeSeasonDatabase::Write(wstring file) {
  wstring folder = Taiga.GetDataPath() + L"db\\season\\";
  file = folder + file;

  xml_document doc;
  xml_node season = doc.append_child();
  season.set_name(L"season");

  // Info
  xml_node info = season.append_child();
  info.set_name(L"info");
  XML_WriteStrValue(info, L"name", Name.c_str());

  // Items
  for (auto it = Items.begin(); it != Items.end(); ++it) {
    xml_node anime = season.append_child();
    anime.set_name(L"anime");
    XML_WriteIntValue(anime, L"series_animedb_id", it->Series_ID);
    XML_WriteStrValue(anime, L"series_title", it->Series_Title.c_str(), node_cdata);
    XML_WriteStrValue(anime, L"series_synonyms", it->Series_Synonyms.c_str(), node_cdata);
    XML_WriteIntValue(anime, L"series_type", it->Series_Type);
    XML_WriteIntValue(anime, L"series_episodes", it->Series_Episodes);
    XML_WriteIntValue(anime, L"series_status", it->Series_Status);
    XML_WriteStrValue(anime, L"series_start", it->Series_Start.c_str());
    XML_WriteStrValue(anime, L"series_end", it->Series_End.c_str());
    XML_WriteStrValue(anime, L"series_image", it->Series_Image.c_str());
    XML_WriteStrValue(anime, L"genres", it->Genres.c_str());
    XML_WriteStrValue(anime, L"producers", it->Producers.c_str());
    XML_WriteStrValue(anime, L"score", it->Score.c_str());
    XML_WriteStrValue(anime, L"rank", it->Rank.c_str());
    XML_WriteStrValue(anime, L"popularity", it->Popularity.c_str());
    XML_WriteStrValue(anime, L"synopsis", it->Synopsis.c_str());
  }

  ::CreateDirectory(folder.c_str(), NULL);
  return doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
}