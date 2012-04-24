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
#include <ctime>

#include "anime.h"
#include "anime_db.h"

#include "common.h"
#include "myanimelist.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "xml.h"

anime::Database AnimeDatabase;
anime::SeasonDatabase SeasonDatabase;

namespace anime {

// =============================================================================

Database::Database()
    : current_id_(ID_UNKNOWN) {
  folder_ = Taiga.GetDataPath() + L"db\\";
  file_ = L"anime.xml";
}

bool Database::LoadDatabase() {
  // Initialize
  wstring path = folder_ + file_;
  
  // Load XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(path.c_str());
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
    items.at(i).SetId(XML_ReadIntValue(node, L"series_animedb_id"));
    items.at(i).SetTitle(XML_ReadStrValue(node, L"series_title"));
    items.at(i).SetSynonyms(XML_ReadStrValue(node, L"series_synonyms"));
    items.at(i).SetType(XML_ReadIntValue(node, L"series_type"));
    items.at(i).SetEpisodeCount(XML_ReadIntValue(node, L"series_episodes"));
    items.at(i).SetAiringStatus(XML_ReadIntValue(node, L"series_status"));
    items.at(i).SetDate(DATE_START, Date(XML_ReadStrValue(node, L"series_start")));
    items.at(i).SetDate(DATE_END, Date(XML_ReadStrValue(node, L"series_end")));
    items.at(i).SetImageUrl(XML_ReadStrValue(node, L"series_image"));
    items.at(i).SetGenres(XML_ReadStrValue(node, L"genres"));
    items.at(i).SetProducers(XML_ReadStrValue(node, L"producers"));
    items.at(i).SetScore(XML_ReadStrValue(node, L"score"));
    items.at(i).SetRank(XML_ReadStrValue(node, L"rank"));
    items.at(i).SetPopularity(XML_ReadStrValue(node, L"popularity"));
    items.at(i).SetSynopsis(XML_ReadStrValue(node, L"synopsis"));
    items.at(i++).last_modified = _wtoi64(XML_ReadStrValue(node, L"last_modified").c_str());
  }

  return true;
}

bool Database::SaveDatabase() {
  if (items.empty()) return false;

  // Initialize
  xml_document doc;
  xml_node animedb_node = doc.append_child();
  animedb_node.set_name(L"animedb");

  // Sort items by their ID
  std::sort(items.begin(), items.end(), 
    [](const Item& item1, const Item& item2) {
      return item1.GetId() < item2.GetId();
    });

  // Write items
  for (auto it = items.begin(); it != items.end(); ++it) {
    xml_node anime_node = animedb_node.append_child();
    anime_node.set_name(L"anime");
    #define XML_WI(n, v) \
      if (v) XML_WriteIntValue(anime_node, n, v)
    #define XML_WS(n, v, t) \
      if (!v.empty()) XML_WriteStrValue(anime_node, n, v.c_str(), t)
    XML_WI(L"series_animedb_id", it->GetId());
    XML_WS(L"series_title", it->GetTitle(), node_cdata);
    XML_WS(L"series_synonyms", Join(it->GetSynonyms(), L"; "), node_cdata);
    XML_WI(L"series_type", it->GetType());
    XML_WI(L"series_episodes", it->GetEpisodeCount());
    XML_WI(L"series_status", it->GetAiringStatus());
    XML_WS(L"series_start", wstring(it->GetDate(DATE_START)), node_pcdata);
    XML_WS(L"series_end", wstring(it->GetDate(DATE_END)), node_pcdata);
    XML_WS(L"series_image", it->GetImageUrl(), node_pcdata);
    XML_WS(L"genres", it->GetGenres(), node_pcdata);
    XML_WS(L"producers", it->GetProducers(), node_pcdata);
    XML_WS(L"score", it->GetScore(), node_pcdata);
    XML_WS(L"rank", it->GetRank(), node_pcdata);
    XML_WS(L"popularity", it->GetPopularity(), node_pcdata);
    XML_WS(L"synopsis", it->GetSynopsis(), node_cdata), node_pcdata;
    XML_WS(L"last_modified", ToWstr(it->last_modified), node_pcdata);
    #undef XML_WS
    #undef XML_WI
  }

  // Save
  CreateFolder(folder_);
  wstring file = folder_ + file_;
  return doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
}

Item* Database::FindItem(int anime_id) {
  if (anime_id > ID_UNKNOWN)
    for (size_t i = 0; i < items.size(); i++)
      if (items.at(i).GetId() == anime_id)
        return &items.at(i);

  return nullptr;
}

int Database::FindItemIndex(int anime_id) const {
  if (anime_id > ID_UNKNOWN)
    for (size_t i = 0; i < items.size(); i++)
      if (items.at(i).GetId() == anime_id)
        return i;

  return -1;
}

void Database::UpdateItem(Item& new_item) {
  critical_section_.Enter();
  
  Item* item = FindItem(new_item.GetId());
  if (!item) {
    // Add a new item
    items.push_back(new_item);
    item = &items.back();
  }

  // Update series information if new information is, well, new.
  if (!item->last_modified || new_item.last_modified > item->last_modified) {
    item->SetId(new_item.GetId());
    item->last_modified = new_item.last_modified;
    
    // Update only if a value is non-empty
    if (new_item.GetType() > 0)
      item->SetType(new_item.GetType());
    if (new_item.GetEpisodeCount() > 0)
      item->SetEpisodeCount(new_item.GetEpisodeCount());
    if (new_item.GetAiringStatus() > 0)
      item->SetAiringStatus(new_item.GetAiringStatus());
    if (!new_item.GetTitle().empty())
      item->SetTitle(new_item.GetTitle());
    if (!new_item.GetSynonyms().empty())
      item->SetSynonyms(new_item.GetSynonyms());
    if (mal::IsValidDate(new_item.GetDate(DATE_START)))
      item->SetDate(DATE_START, new_item.GetDate(DATE_START));
    if (mal::IsValidDate(new_item.GetDate(DATE_END)))
      item->SetDate(DATE_END, new_item.GetDate(DATE_END));
    if (!new_item.GetImageUrl().empty())
      item->SetImageUrl(new_item.GetImageUrl());
    if (!new_item.GetGenres().empty())
      item->SetGenres(new_item.GetGenres());
    if (!new_item.GetPopularity().empty())
      item->SetPopularity(new_item.GetPopularity());
    if (!new_item.GetProducers().empty())
      item->SetProducers(new_item.GetProducers());
    if (!new_item.GetRank().empty())
      item->SetRank(new_item.GetRank());
    if (!new_item.GetScore().empty())
      item->SetScore(new_item.GetScore());
    if (!new_item.GetSynopsis().empty())
      item->SetSynopsis(new_item.GetSynopsis());
  }

  // Update user information
  if (new_item.IsInList()) {
    // Make sure our pointer to MyInformation class is valid
    item->AddtoUserList();

    item->SetMyLastWatchedEpisode(new_item.GetMyLastWatchedEpisode(false));
    item->SetMyScore(new_item.GetMyScore(false));
    item->SetMyStatus(new_item.GetMyStatus(false));
    item->SetMyRewatching(new_item.GetMyRewatching(false));
    item->SetMyRewatchingEp(new_item.GetMyRewatchingEp());
    item->SetMyDate(DATE_START, new_item.GetMyDate(DATE_START), true, false);
    item->SetMyDate(DATE_END, new_item.GetMyDate(DATE_END), true, false);
    item->SetMyLastUpdated(new_item.GetMyLastUpdated());
    item->SetMyTags(new_item.GetMyTags(false));
    
    if (item->GetEpisodeCount() > 0)
      item->SetEpisodeAvailability(item->GetEpisodeCount(), false, L"");

    user.IncreaseItemCount(item->GetMyStatus(false), false);

    for (size_t i = 0; i < Settings.Anime.items.size(); i++) {
      if (item->GetId() == Settings.Anime.items[i].id) {
        item->SetFolder(Settings.Anime.items[i].folder, false);
        item->SetUserSynonyms(Settings.Anime.items[i].titles, false);
        break;
      }
    }
  }

  critical_section_.Leave();
}

// =============================================================================

void Database::ClearUserData() {
  current_id_ = ID_UNKNOWN;
  
  for (auto it = items.begin(); it != items.end(); ++it) {
    it->RemoveFromUserList();
  }
  
  user.Clear();
}

bool Database::LoadList() {
  // Initialize
  ClearUserData();
  if (Settings.Account.MAL.user.empty()) return false;
  wstring file = Taiga.GetDataPath() + L"user\\" + Settings.Account.MAL.user + L".xml";
  
  // Load XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != status_ok && result.status != status_file_not_found) {
    MessageBox(NULL, L"Could not read anime list.", file.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  // Read user info
  xml_node myanimelist = doc.child(L"myanimelist");
  xml_node myinfo = myanimelist.child(L"myinfo");
  user.SetId(XML_ReadIntValue(myinfo, L"user_id"));
  user.SetName(XML_ReadStrValue(myinfo, L"user_name"));
  // Since MAL can be too slow to update these values, we'll be counting by 
  // ourselves at Database::UpdateItem().
  /*
  user.SetItemCount(mal::MYSTATUS_WATCHING, 
    XML_ReadIntValue(myinfo, L"user_watching"), false);
  user.SetItemCount(mal::MYSTATUS_COMPLETED, 
    XML_ReadIntValue(myinfo, L"user_completed"), false);
  user.SetItemCount(mal::MYSTATUS_ONHOLD, 
    XML_ReadIntValue(myinfo, L"user_onhold"), false);
  user.SetItemCount(mal::MYSTATUS_DROPPED, 
    XML_ReadIntValue(myinfo, L"user_dropped"), false);
  user.SetItemCount(mal::MYSTATUS_PLANTOWATCH, 
    XML_ReadIntValue(myinfo, L"user_plantowatch"), false);
  */
  user.SetDaysSpentWatching(XML_ReadStrValue(myinfo, L"user_days_spent_watching"));

  // Read anime list
  for (xml_node node = myanimelist.child(L"anime"); node; node = node.next_sibling(L"anime")) {
    Item anime_item;
    anime_item.SetId(XML_ReadIntValue(node, L"series_animedb_id"));
    anime_item.SetTitle(DecodeHtml(XML_ReadStrValue(node, L"series_title")));
    anime_item.SetSynonyms(DecodeHtml(XML_ReadStrValue(node, L"series_synonyms")));
    anime_item.SetType(XML_ReadIntValue(node, L"series_type"));
    anime_item.SetEpisodeCount(XML_ReadIntValue(node, L"series_episodes"));
    anime_item.SetAiringStatus(XML_ReadIntValue(node, L"series_status"));
    anime_item.SetDate(DATE_START, XML_ReadStrValue(node, L"series_start"));
    anime_item.SetDate(DATE_END, XML_ReadStrValue(node, L"series_end"));
    anime_item.SetImageUrl(XML_ReadStrValue(node, L"series_image"));
    anime_item.AddtoUserList();
    anime_item.SetMyLastWatchedEpisode(XML_ReadIntValue(node, L"my_watched_episodes"));
    anime_item.SetMyDate(DATE_START, XML_ReadStrValue(node, L"my_start_date"), true, false);
    anime_item.SetMyDate(DATE_END, XML_ReadStrValue(node, L"my_finish_date"), true, false);
    anime_item.SetMyScore(XML_ReadIntValue(node, L"my_score"));
    anime_item.SetMyStatus(XML_ReadIntValue(node, L"my_status"));
    anime_item.SetMyRewatching(XML_ReadIntValue(node, L"my_rewatching"));
    anime_item.SetMyRewatchingEp(XML_ReadIntValue(node, L"my_rewatching_ep"));
    anime_item.SetMyLastUpdated(XML_ReadStrValue(node, L"my_last_updated"));
    anime_item.SetMyTags(XML_ReadStrValue(node, L"my_tags"));
    UpdateItem(anime_item);
  }

  return true;
}

bool Database::SaveList(int anime_id, const wstring& child, const wstring& value, ListSaveMode mode) {
  auto item = FindItem(anime_id);

  if (mode != EDIT_USER && !item) {
    return false;
  }
  
  // Initialize
  wstring folder = Taiga.GetDataPath() + L"user\\";
  wstring file = folder + Settings.Account.MAL.user + L".xml";
  if (!PathExists(folder)) CreateFolder(folder);
  
  // Load XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != status_ok) return false;

  // Read anime list
  xml_node myanimelist = doc.child(L"myanimelist");
  switch (mode) {
    // Add anime item
    case ADD_ANIME: {
      xml_node node = myanimelist.append_child();
      node.set_name(L"anime");
      XML_WriteIntValue(node, L"series_animedb_id", item->GetId());
      XML_WriteIntValue(node, L"my_watched_episodes", item->GetMyLastWatchedEpisode(false));
      XML_WriteStrValue(node, L"my_start_date", wstring(item->GetDate(DATE_START)).c_str());
      XML_WriteStrValue(node, L"my_finish_date", wstring(item->GetDate(DATE_END)).c_str());
      XML_WriteIntValue(node, L"my_score", item->GetMyScore(false));
      XML_WriteIntValue(node, L"my_status", item->GetMyStatus(false));
      XML_WriteIntValue(node, L"my_rewatching", item->GetMyRewatching(false));
      XML_WriteIntValue(node, L"my_rewatching_ep", item->GetMyRewatchingEp());
      XML_WriteStrValue(node, L"my_last_updated", item->GetMyLastUpdated().c_str());
      XML_WriteStrValue(node, L"my_tags", item->GetMyTags(false).c_str());
      doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
      return true;
    }
    // Delete anime item
    case DELETE_ANIME: {
      for (xml_node node = myanimelist.child(L"anime"); node; node = node.next_sibling(L"anime")) {
        if (XML_ReadIntValue(node, L"series_animedb_id") == item->GetId()) {
          myanimelist.remove_child(node);
          doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
          return true;
        }
      }
      break;
    }
    // Edit anime data
    case EDIT_ANIME: {
      for (xml_node node = myanimelist.child(L"anime"); node; node = node.next_sibling(L"anime")) {
        if (XML_ReadIntValue(node, L"series_animedb_id") == item->GetId()) {
          xml_node child_node = node.child(child.c_str());
          if (wstring(child_node.first_child().value()).empty()) {
            child_node = child_node.append_child(node_pcdata);
          } else {
            child_node = child_node.first_child();
          }
          child_node.set_value(value.c_str());
          doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
          return true;
        }
      }
      break;
    }
    // Edit user data
    case EDIT_USER: {
      myanimelist.child(L"myinfo").child(child.c_str()).first_child().set_value(value.c_str());
      doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
      return true;
    }
  }

  return false;
}

// =============================================================================

bool Database::DeleteItem(int anime_id) {
  auto item = FindItem(anime_id);
  if (!item) return false;
  if (!item->IsInList()) return false;
  
  user.DecreaseItemCount(item->GetMyStatus(false), true);
  SaveList(anime_id, L"", L"", DELETE_ANIME);

  /*
  items.erase(items.begin() + item->index);
  for (size_t i = 0; i < items.size(); i++) {
    items.at(i).index = i;
  }
  */

  item->RemoveFromUserList();

  return true;
}

int Database::GetCurrentId() {
  if (current_id_ > ID_UNKNOWN) {
    auto item = FindItem(current_id_);
    if (!item) current_id_ = ID_UNKNOWN;
  }

  return current_id_;
}

Item* Database::GetCurrentItem() {
  Item* item = nullptr;

  if (current_id_ > ID_UNKNOWN) {
    item = FindItem(current_id_);
    if (!item) current_id_ = ID_UNKNOWN;
  }

  return item;
}

void Database::SetCurrentId(int anime_id) {
  // may not be necessary
  if (anime_id > ID_UNKNOWN) {
    auto item = FindItem(anime_id);
    if (!item) anime_id = ID_UNKNOWN;
  }

  current_id_ = anime_id;
}

// =============================================================================

SeasonDatabase::SeasonDatabase()
    : last_modified(0) {
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
  if (result.status != status_ok && result.status != status_file_not_found) {
    MessageBox(NULL, L"Could not read season data.", file.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  // Read information
  xml_node season_node = doc.child(L"season");
  name = XML_ReadStrValue(season_node.child(L"info"), L"name");
  last_modified = _wtoi64(XML_ReadStrValue(season_node.child(L"info"), L"last_modified").c_str());

  // Read items
  for (xml_node node = season_node.child(L"anime"); node; node = node.next_sibling(L"anime")) {
    int anime_id = XML_ReadIntValue(node, L"series_animedb_id");
    items.push_back(anime_id);
    
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item && anime_item->last_modified >= last_modified) {
      continue;
    }

    Item item;
    item.SetId(XML_ReadIntValue(node, L"series_animedb_id"));
    item.SetTitle(XML_ReadStrValue(node, L"series_title"));
    item.SetType(XML_ReadIntValue(node, L"series_type"));
    item.SetImageUrl(XML_ReadStrValue(node, L"series_image"));
    item.SetProducers(XML_ReadStrValue(node, L"producers"));
    xml_node settings_node = node.child(L"settings");
    item.keep_title = XML_ReadIntValue(settings_node, L"keep_title") != 0;
    item.last_modified = last_modified;
    AnimeDatabase.UpdateItem(item);
  }

  return true;
}

} // namespace anime