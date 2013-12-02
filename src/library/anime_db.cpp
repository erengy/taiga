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

#include "base/std.h"
#include <ctime>

#include "anime.h"
#include "anime_db.h"
#include "history.h"

#include "base/common.h"
#include "base/foreach.h"
#include "base/logger.h"
#include "sync/myanimelist.h"
#include "sync/sync.h"
#include "track/recognition.h"
#include "taiga/settings.h"
#include "base/string.h"
#include "taiga/taiga.h"
#include "base/xml.h"

#include "ui/dlg/dlg_anime_info.h"
#include "ui/dlg/dlg_anime_list.h"
#include "ui/dlg/dlg_season.h"

anime::Database AnimeDatabase;
anime::ImageDatabase ImageDatabase;
anime::SeasonDatabase SeasonDatabase;

namespace anime {

// =============================================================================

Database::Database() {
  folder_ = Taiga.GetDataPath() + L"db\\";
  file_ = L"anime.xml";
}

bool Database::LoadDatabase() {
  // Initialize
  wstring path = folder_ + file_;
  
  // Load XML file
  xml_document doc;
  unsigned int options = pugi::parse_default & ~pugi::parse_eol;
  xml_parse_result result = doc.load_file(path.c_str(), options);
  if (result.status != pugi::status_ok && result.status != pugi::status_file_not_found) {
    return false;
  }

  // Read items
  xml_node animedb_node = doc.child(L"animedb");
  for (xml_node node = animedb_node.child(L"anime"); node; node = node.next_sibling(L"anime")) {
    int id = XmlReadIntValue(node, L"series_animedb_id");
    Item& item = items[id]; // Creates the item if it doesn't exist
    item.SetId(id);
    item.SetTitle(XmlReadStrValue(node, L"series_title"));
    item.SetEnglishTitle(XmlReadStrValue(node, L"series_english"));
    item.SetSynonyms(XmlReadStrValue(node, L"series_synonyms"));
    item.SetType(XmlReadIntValue(node, L"series_type"));
    item.SetEpisodeCount(XmlReadIntValue(node, L"series_episodes"));
    item.SetAiringStatus(XmlReadIntValue(node, L"series_status"));
    item.SetDate(DATE_START, Date(XmlReadStrValue(node, L"series_start")));
    item.SetDate(DATE_END, Date(XmlReadStrValue(node, L"series_end")));
    item.SetImageUrl(XmlReadStrValue(node, L"series_image"));
    item.SetGenres(XmlReadStrValue(node, L"genres"));
    item.SetProducers(XmlReadStrValue(node, L"producers"));
    item.SetScore(XmlReadStrValue(node, L"score"));
    item.SetRank(XmlReadStrValue(node, L"rank"));
    item.SetPopularity(XmlReadStrValue(node, L"popularity"));
    item.SetSynopsis(XmlReadStrValue(node, L"synopsis"));
    item.last_modified = _wtoi64(XmlReadStrValue(node, L"last_modified").c_str());
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
      if (v > 0) XmlWriteIntValue(anime_node, n, v)
    #define XML_WS(n, v, t) \
      if (!v.empty()) XmlWriteStrValue(anime_node, n, v.c_str(), t)
    XML_WI(L"series_animedb_id", it->second.GetId());
    XML_WS(L"series_title", it->second.GetTitle(), pugi::node_cdata);
    XML_WS(L"series_english", it->second.GetEnglishTitle(), pugi::node_cdata);
    XML_WS(L"series_synonyms", Join(it->second.GetSynonyms(), L"; "), pugi::node_cdata);
    XML_WI(L"series_type", it->second.GetType());
    XML_WI(L"series_episodes", it->second.GetEpisodeCount());
    XML_WI(L"series_status", it->second.GetAiringStatus());
    XML_WS(L"series_start", wstring(it->second.GetDate(DATE_START)), pugi::node_pcdata);
    XML_WS(L"series_end", wstring(it->second.GetDate(DATE_END)), pugi::node_pcdata);
    XML_WS(L"series_image", it->second.GetImageUrl(), pugi::node_pcdata);
    XML_WS(L"genres", it->second.GetGenres(), pugi::node_pcdata);
    XML_WS(L"producers", it->second.GetProducers(), pugi::node_pcdata);
    XML_WS(L"score", it->second.GetScore(), pugi::node_pcdata);
    XML_WS(L"rank", it->second.GetRank(), pugi::node_pcdata);
    XML_WS(L"popularity", it->second.GetPopularity(), pugi::node_pcdata);
    XML_WS(L"synopsis", it->second.GetSynopsis(), pugi::node_cdata);
    XML_WS(L"last_modified", ToWstr(it->second.last_modified), pugi::node_pcdata);
    #undef XML_WS
    #undef XML_WI
  }

  // Save
  CreateFolder(folder_);
  wstring file = folder_ + file_;
  return doc.save_file(file.c_str(), L"\x09", pugi::format_default | pugi::format_write_bom);
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

int Database::GetItemCount(int status, bool check_events) {
  int count = 0;

  // Get current count
  foreach_(it, items)
    if (it->second.GetMyStatus(false) == status)
      count++;

  // Search event queue for status changes
  if (check_events) {
    foreach_(it, History.queue.items) {
      if (it->mode == taiga::kHttpServiceAddLibraryEntry)
        continue;
      if (it->status) {
        if (status == *it->status) {
          count++;
        } else {
          auto anime_item = FindItem(it->anime_id);
          if (anime_item && status == anime_item->GetMyStatus(false))
            count--;
        }
      }
    }
  }

  return count;
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
    if (sync::myanimelist::IsValidDate(new_item.GetDate(DATE_START)))
      item->SetDate(DATE_START, new_item.GetDate(DATE_START));
    if (sync::myanimelist::IsValidDate(new_item.GetDate(DATE_END)))
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
  }

  critical_section_.Leave();
}

// =============================================================================

void Database::ClearUserData() {
  AnimeListDialog.SetCurrentId(ID_UNKNOWN);
  
  for (auto it = items.begin(); it != items.end(); ++it) {
    it->second.RemoveFromUserList();
  }
}

void Database::ClearInvalidItems() {
  for (auto it = items.begin(); it != items.end(); ) {
    if (!it->second.GetId() || it->first != it->second.GetId()) {
      LOG(LevelDebug, L"ID: " + ToWstr(it->first));
      items.erase(it++);
    } else {
      ++it;
    }
  }
}

bool Database::DeleteListItem(int anime_id) {
  auto item = FindItem(anime_id);
  if (!item) return false;
  if (!item->IsInList()) return false;

  item->RemoveFromUserList();

  return true;
}

bool Database::LoadList() {
  // Initialize
  ClearUserData();
  if (Settings.Account.MAL.user.empty()) return false;
  wstring file = Taiga.GetDataPath() + 
    L"user\\" + Settings.Account.MAL.user + L"\\anime.xml";
  
  // Load XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != pugi::status_ok && result.status != pugi::status_file_not_found) {
    MessageBox(NULL, L"Could not read anime list.", file.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  xml_node myanimelist = doc.child(L"myanimelist");

  // Read anime list
  for (xml_node node = myanimelist.child(L"anime"); node; node = node.next_sibling(L"anime")) {
    Item anime_item;
    anime_item.SetId(XmlReadIntValue(node, L"series_animedb_id"));
    anime_item.SetTitle(XmlReadStrValue(node, L"series_title"));
    anime_item.SetSynonyms(XmlReadStrValue(node, L"series_synonyms"));
    anime_item.SetType(XmlReadIntValue(node, L"series_type"));
    anime_item.SetEpisodeCount(XmlReadIntValue(node, L"series_episodes"));
    anime_item.SetAiringStatus(XmlReadIntValue(node, L"series_status"));
    anime_item.SetDate(DATE_START, XmlReadStrValue(node, L"series_start"));
    anime_item.SetDate(DATE_END, XmlReadStrValue(node, L"series_end"));
    anime_item.SetImageUrl(XmlReadStrValue(node, L"series_image"));
    anime_item.AddtoUserList();
    anime_item.SetMyLastWatchedEpisode(XmlReadIntValue(node, L"my_watched_episodes"));
    anime_item.SetMyDate(DATE_START, XmlReadStrValue(node, L"my_start_date"));
    anime_item.SetMyDate(DATE_END, XmlReadStrValue(node, L"my_finish_date"));
    anime_item.SetMyScore(XmlReadIntValue(node, L"my_score"));
    anime_item.SetMyStatus(XmlReadIntValue(node, L"my_status"));
    anime_item.SetMyRewatching(XmlReadIntValue(node, L"my_rewatching"));
    anime_item.SetMyRewatchingEp(XmlReadIntValue(node, L"my_rewatching_ep"));
    anime_item.SetMyLastUpdated(XmlReadStrValue(node, L"my_last_updated"));
    anime_item.SetMyTags(XmlReadStrValue(node, L"my_tags"));
    UpdateItem(anime_item);
  }

  return true;
}

bool Database::SaveList() {
  if (items.empty())
    return false;

  // Initialize
  xml_document document;
  xml_node myanimelist_node = document.append_child(L"myanimelist");

  // Write items
  foreach_(it, items) {
    Item* item = &it->second;
    if (item->IsInList()) {
      xml_node node = myanimelist_node.append_child(L"anime");
      XmlWriteIntValue(node, L"series_animedb_id", item->GetId());
      XmlWriteIntValue(node, L"my_watched_episodes", item->GetMyLastWatchedEpisode(false));
      XmlWriteStrValue(node, L"my_start_date", wstring(item->GetMyDate(DATE_START)).c_str());
      XmlWriteStrValue(node, L"my_finish_date", wstring(item->GetMyDate(DATE_END)).c_str());
      XmlWriteIntValue(node, L"my_score", item->GetMyScore(false));
      XmlWriteIntValue(node, L"my_status", item->GetMyStatus(false));
      XmlWriteIntValue(node, L"my_rewatching", item->GetMyRewatching(false));
      XmlWriteIntValue(node, L"my_rewatching_ep", item->GetMyRewatchingEp());
      XmlWriteStrValue(node, L"my_last_updated", item->GetMyLastUpdated().c_str());
      XmlWriteStrValue(node, L"my_tags", item->GetMyTags(false).c_str());
    }
  }

  // Save
  wstring folder = Taiga.GetDataPath() + L"user\\" + Settings.Account.MAL.user + L"\\";
  if (!PathExists(folder)) CreateFolder(folder);
  wstring file = folder + L"anime.xml";
  return document.save_file(file.c_str(), L"\x09", pugi::format_default | pugi::format_write_bom);
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
  if (result.status != pugi::status_ok && result.status != pugi::status_file_not_found) {
    MessageBox(NULL, L"Could not read fansub data.", file.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  // Read items
  xml_node fansub_node = doc.child(L"fansub_groups");
  for (xml_node node = fansub_node.child(L"fansub"); node; node = node.next_sibling(L"fansub")) {
    items.push_back(XmlReadStrValue(node, L"name"));
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
      if (anime_item->GetAiringStatus() != sync::myanimelist::kFinishedAiring) {
        // Check last modified date (>= 7 days)
        if (GetFileAge(anime::GetImagePath(anime_id)) / (60 * 60 * 24) >= 7) {
          sync::DownloadImage(anime_id, anime_item->GetImageUrl());
        }
      }
    }
    return true;
  } else {
    items_[anime_id].data = -1;
  }

  if (download) {
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    sync::DownloadImage(anime_id, anime_item->GetImageUrl());
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

    if (erase)
      items_.erase(anime_id);
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

    Item item;
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
      if (!sync::myanimelist::IsValidDate(date_start) || anime_item->GetSynopsis().empty())
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
  sync::myanimelist::GetSeasonInterval(name, date_start, date_end);

  // Check for invalid items
  for (size_t i = 0; i < items.size(); i++) {
    bool invalid = false;
    int anime_id = items.at(i);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item) {
      // Airing date must be within the interval
      const Date& anime_start = anime_item->GetDate(anime::DATE_START);
      if (sync::myanimelist::IsValidDate(anime_start))
        if (anime_start < date_start || anime_start > date_end)
          invalid = true;
      // TODO: Filter by rating instead if made possible in API
      if (hide_hentai && InStr(anime_item->GetGenres(), L"Hentai", 0, true) > -1)
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
    if (hide_hentai && InStr(it->second.GetGenres(), L"Hentai", 0, true) > -1)
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

} // namespace anime