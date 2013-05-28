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
#include <ctime>

#include "anime.h"
#include "anime_db.h"

#include "common.h"
#include "debug.h"
#include "myanimelist.h"
#include "recognition.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "xml.h"

#include "dlg/dlg_anime_info.h"
#include "dlg/dlg_anime_list.h"
#include "dlg/dlg_season.h"

anime::Database AnimeDatabase;
anime::ImageDatabase ImageDatabase;
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
  xml_node animedb_node = doc.child(L"animedb");
  for (xml_node node = animedb_node.child(L"anime"); node; node = node.next_sibling(L"anime")) {
    Item item;
    item.SetId(XML_ReadIntValue(node, L"series_animedb_id"));
    item.SetTitle(XML_ReadStrValue(node, L"series_title"));
    item.SetEnglishTitle(XML_ReadStrValue(node, L"series_english"));
    item.SetSynonyms(XML_ReadStrValue(node, L"series_synonyms"));
    item.SetType(XML_ReadIntValue(node, L"series_type"));
    item.SetEpisodeCount(XML_ReadIntValue(node, L"series_episodes"));
    item.SetAiringStatus(XML_ReadIntValue(node, L"series_status"));
    item.SetDate(DATE_START, Date(XML_ReadStrValue(node, L"series_start")));
    item.SetDate(DATE_END, Date(XML_ReadStrValue(node, L"series_end")));
    item.SetImageUrl(XML_ReadStrValue(node, L"series_image"));
    item.SetGenres(XML_ReadStrValue(node, L"genres"));
    item.SetProducers(XML_ReadStrValue(node, L"producers"));
    item.SetScore(XML_ReadStrValue(node, L"score"));
    item.SetRank(XML_ReadStrValue(node, L"rank"));
    item.SetPopularity(XML_ReadStrValue(node, L"popularity"));
    item.SetSynopsis(XML_ReadStrValue(node, L"synopsis"));
    item.last_modified = _wtoi64(XML_ReadStrValue(node, L"last_modified").c_str());
    items[item.GetId()] = item;
  }

  return true;
}

bool Database::SaveDatabase() {
  if (items.empty()) return false;

  // Initialize
  xml_document doc;
  xml_node animedb_node = doc.append_child(L"animedb");

  // Write items
  for (auto it = items.begin(); it != items.end(); ++it) {
    xml_node anime_node = animedb_node.append_child(L"anime");
    #define XML_WI(n, v) \
      if (v > 0) XML_WriteIntValue(anime_node, n, v)
    #define XML_WS(n, v, t) \
      if (!v.empty()) XML_WriteStrValue(anime_node, n, v.c_str(), t)
    XML_WI(L"series_animedb_id", it->second.GetId());
    XML_WS(L"series_title", it->second.GetTitle(), node_cdata);
    XML_WS(L"series_english", it->second.GetEnglishTitle(), node_cdata);
    XML_WS(L"series_synonyms", Join(it->second.GetSynonyms(), L"; "), node_cdata);
    XML_WI(L"series_type", it->second.GetType());
    XML_WI(L"series_episodes", it->second.GetEpisodeCount());
    XML_WI(L"series_status", it->second.GetAiringStatus());
    XML_WS(L"series_start", wstring(it->second.GetDate(DATE_START)), node_pcdata);
    XML_WS(L"series_end", wstring(it->second.GetDate(DATE_END)), node_pcdata);
    XML_WS(L"series_image", it->second.GetImageUrl(), node_pcdata);
    XML_WS(L"genres", it->second.GetGenres(), node_pcdata);
    XML_WS(L"producers", it->second.GetProducers(), node_pcdata);
    XML_WS(L"score", it->second.GetScore(), node_pcdata);
    XML_WS(L"rank", it->second.GetRank(), node_pcdata);
    XML_WS(L"popularity", it->second.GetPopularity(), node_pcdata);
    XML_WS(L"synopsis", it->second.GetSynopsis(), node_cdata), node_pcdata;
    XML_WS(L"last_modified", ToWstr(it->second.last_modified), node_pcdata);
    #undef XML_WS
    #undef XML_WI
  }

  // Save
  CreateFolder(folder_);
  wstring file = folder_ + file_;
  return doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
}

Item* Database::FindItem(int anime_id) {
  if (anime_id > ID_UNKNOWN) {
    auto it = items.find(anime_id);
    if (it != items.end()) return &it->second;
  }

  return nullptr;
}

Item* Database::FindSequel(int anime_id) {
  int sequel_id = ID_UNKNOWN;

  switch (anime_id) {
    // Gintama -> Gintama'
    case 918: sequel_id = 9969; break;
    // Tegami Bachi -> Tegami Bachi Reverse
    case 6444: sequel_id = 8311; break;
    // Fate/Zero -> Fate/Zero 2nd Season
    case 10087: sequel_id = 11741; break;
    // Towa no Qwon
    case 10294: sequel_id = 10713; break;
    case 10713: sequel_id = 10714; break;
    case 10714: sequel_id = 10715; break;
    case 10715: sequel_id = 10716; break;
    case 10716: sequel_id = 10717; break;
  }

  return FindItem(sequel_id);
}

void Database::UpdateItem(Item& new_item) {
  critical_section_.Enter();
  
  Item* item = FindItem(new_item.GetId());
  if (!item) {
    // Add a new item
    item = &items[new_item.GetId()];
  }

  // Update series information if new information is, well, new.
  if (!item->last_modified || new_item.last_modified > item->last_modified) {
    item->SetId(new_item.GetId());
    item->last_modified = new_item.last_modified;

    // Update only if a value is non-empty
    if (new_item.GetType() > 0)
      item->SetType(new_item.GetType());
    if (new_item.GetEpisodeCount(false) > -1)
      item->SetEpisodeCount(new_item.GetEpisodeCount());
    if (new_item.GetAiringStatus(false) > 0)
      item->SetAiringStatus(new_item.GetAiringStatus());
    if (!new_item.GetTitle().empty())
      item->SetTitle(new_item.GetTitle());
    if (!new_item.GetEnglishTitle(false).empty())
      item->SetEnglishTitle(new_item.GetEnglishTitle());
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

    // Update clean titles, if necessary
    if (!new_item.GetTitle().empty() ||
        !new_item.GetSynonyms().empty() ||
        !new_item.GetEnglishTitle(false).empty())
      Meow.UpdateCleanTitles(new_item.GetId());
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
    item->SetMyDate(DATE_START, new_item.GetMyDate(DATE_START));
    item->SetMyDate(DATE_END, new_item.GetMyDate(DATE_END));
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
    it->second.RemoveFromUserList();
  }
  
  user.Clear();
}

bool Database::LoadList(bool set_last_modified) {
  // Initialize
  ClearUserData();
  if (Settings.Account.MAL.user.empty()) return false;
  wstring file = Taiga.GetDataPath() + 
    L"user\\" + Settings.Account.MAL.user + L"\\anime.xml";
  time_t last_modified = set_last_modified ? time(nullptr) : 0;
  
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
    anime_item.SetTitle(XML_ReadStrValue(node, L"series_title"));
    anime_item.SetSynonyms(XML_ReadStrValue(node, L"series_synonyms"));
    anime_item.SetType(XML_ReadIntValue(node, L"series_type"));
    anime_item.SetEpisodeCount(XML_ReadIntValue(node, L"series_episodes"));
    anime_item.SetAiringStatus(XML_ReadIntValue(node, L"series_status"));
    anime_item.SetDate(DATE_START, XML_ReadStrValue(node, L"series_start"));
    anime_item.SetDate(DATE_END, XML_ReadStrValue(node, L"series_end"));
    anime_item.SetImageUrl(XML_ReadStrValue(node, L"series_image"));
    anime_item.last_modified = last_modified;
    anime_item.AddtoUserList();
    anime_item.SetMyLastWatchedEpisode(XML_ReadIntValue(node, L"my_watched_episodes"));
    anime_item.SetMyDate(DATE_START, XML_ReadStrValue(node, L"my_start_date"));
    anime_item.SetMyDate(DATE_END, XML_ReadStrValue(node, L"my_finish_date"));
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
  wstring folder = Taiga.GetDataPath() + L"user\\" + Settings.Account.MAL.user + L"\\";
  wstring file = folder + L"anime.xml";
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
      xml_node node = myanimelist.append_child(L"anime");
      XML_WriteIntValue(node, L"series_animedb_id", item->GetId());
      XML_WriteIntValue(node, L"my_watched_episodes", item->GetMyLastWatchedEpisode(false));
      XML_WriteStrValue(node, L"my_start_date", wstring(item->GetMyDate(DATE_START)).c_str());
      XML_WriteStrValue(node, L"my_finish_date", wstring(item->GetMyDate(DATE_END)).c_str());
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

bool Database::DeleteListItem(int anime_id) {
  auto item = FindItem(anime_id);
  if (!item) return false;
  if (!item->IsInList()) return false;

  user.DecreaseItemCount(item->GetMyStatus(false), true);
  SaveList(anime_id, L"", L"", DELETE_ANIME);
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

FansubDatabase::FansubDatabase() {
  file_ = L"fansub.xml";
  folder_ = Taiga.GetDataPath() + L"db\\";
}

bool FansubDatabase::Load() {
  // Initialize
  wstring file = folder_ + file_;
  items.clear();
  
  // Load XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != status_ok && result.status != status_file_not_found) {
    MessageBox(NULL, L"Could not read fansub data.", file.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  // Read items
  xml_node fansub_node = doc.child(L"fansub_groups");
  for (xml_node node = fansub_node.child(L"fansub"); node; node = node.next_sibling(L"fansub")) {
    items.push_back(XML_ReadStrValue(node, L"name"));
  }

  return true;
}

void FansubDatabase::Save() {
  // TODO
}

// =============================================================================

bool ImageDatabase::Load(int anime_id, bool load, bool download) {
  if (anime_id <= anime::ID_UNKNOWN)
    return false;
  
  if (items_.find(anime_id) != items_.end()) {
    if (items_[anime_id].data > anime::ID_UNKNOWN) {
      return true;
    } else if (!load) {
      return false;
    }
  }
  
  if (items_[anime_id].Load(anime::GetImagePath(anime_id))) {
    items_[anime_id].data = anime_id;
    if (download) {
      // Refresh if current file is too old
      auto anime_item = AnimeDatabase.FindItem(anime_id);
      if (anime_item->GetAiringStatus() != mal::STATUS_FINISHED) {
        // Check last modified date (>= 7 days)
        if (GetFileAge(anime::GetImagePath(anime_id)) / (60 * 60 * 24) >= 7) {
          mal::DownloadImage(anime_id, anime_item->GetImageUrl());
        }
      }
    }
    return true;
  } else {
    items_[anime_id].data = -1;
  }

  if (download) {
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    mal::DownloadImage(anime_id, anime_item->GetImageUrl());
  }
  
  return false;
}

void ImageDatabase::FreeMemory() {
  for (auto it = ::AnimeDatabase.items.begin(); it != ::AnimeDatabase.items.end(); ++it) {
    bool erase = true;
    int anime_id = it->first;

    if (items_.find(anime_id) == items_.end())
      continue;

    if (::AnimeDialog.GetCurrentId() == anime_id ||
        ::NowPlayingDialog.GetCurrentId() == anime_id)
      erase = false;

    if (!::SeasonDatabase.items.empty())
      if (std::find(::SeasonDatabase.items.begin(), ::SeasonDatabase.items.end(), 
                    anime_id) != ::SeasonDatabase.items.end())
        if (SeasonDialog.IsVisible())
          erase = false;

    if (erase) {
      items_.erase(anime_id);
      debug::Print(L"ImageDatabase::FreeMemory :: Erased item #" + ToWstr(anime_id) + 
                   L" - \"" + it->second.GetTitle() + L"\"\n");
    }
  }
}

Image* ImageDatabase::GetImage(int anime_id) {
  if (items_.find(anime_id) != items_.end())
    if (items_[anime_id].data > 0)
      return &items_[anime_id];
  return nullptr;
}

// =============================================================================

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
  if (result.status != status_ok && result.status != status_file_not_found) {
    MessageBox(NULL, L"Could not read season data.", file.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  // Read information
  xml_node season_node = doc.child(L"season");
  name = XML_ReadStrValue(season_node.child(L"info"), L"name");
  time_t last_modified = _wtoi64(XML_ReadStrValue(season_node.child(L"info"), L"last_modified").c_str());

  // Read items
  for (xml_node node = season_node.child(L"anime"); node; node = node.next_sibling(L"anime")) {
    int anime_id = XML_ReadIntValue(node, L"series_animedb_id");
    items.push_back(anime_id);
    
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item && anime_item->last_modified >= last_modified)
      continue;

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

bool SeasonDatabase::IsRefreshRequired() {
  int count = 0;
  bool required = false;

  for (int i = 0; i < items.size(); i++) {
    int anime_id = items.at(i);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item) {
      const Date& date_start = anime_item->GetDate(anime::DATE_START);
      if (!mal::IsValidDate(date_start) || anime_item->GetSynopsis().empty())
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
  mal::GetSeasonInterval(name, date_start, date_end);

  // Check for invalid items
  for (int i = 0; i < items.size(); i++) {
    bool invalid = false;
    int anime_id = items.at(i);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item) {
      // Airing date must be within the interval
      const Date& anime_start = anime_item->GetDate(anime::DATE_START);
      if (mal::IsValidDate(anime_start))
        if (anime_start < date_start || anime_start > date_end)
          invalid = true;
      // TODO: Filter by rating instead if made possible in API
      if (hide_hentai && InStr(anime_item->GetGenres(), L"Hentai", 0, true) > -1)
        invalid = true;
      if (invalid) {
        items.erase(items.begin() + i--);
        debug::Print(L"SeasonDatabase::Review :: Removed item: \"" + 
                     anime_item->GetTitle() + L"\" (" + 
                     wstring(anime_start) + L")\n");
      }
    }
  }

  // Check for missing items
  for (auto it = AnimeDatabase.items.begin(); it != AnimeDatabase.items.end(); ++it) {
    if (std::find(items.begin(), items.end(), it->second.GetId()) != items.end())
      continue;
    // TODO: Filter by rating instead if made possible in API
    if (hide_hentai && InStr(it->second.GetGenres(), L"Hentai", 0, true) > -1)
      continue;
    // Airing date must be within the interval
    const Date& anime_start = it->second.GetDate(anime::DATE_START);
    if (anime_start.year && anime_start.month &&
        anime_start >= date_start && anime_start <= date_end) {
      items.push_back(it->second.GetId());
      debug::Print(L"SeasonDatabase::Review :: Added item: \"" + 
                   it->second.GetTitle() + L"\" (" + 
                   wstring(anime_start) + L")\n");
    }
  }
}

} // namespace anime