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
#include "anime_util.h"
#include "discover.h"
#include "history.h"

#include "base/common.h"
#include "base/file.h"
#include "base/foreach.h"
#include "base/logger.h"
#include "sync/sync.h"
#include "track/recognition.h"
#include "taiga/settings.h"
#include "base/string.h"
#include "taiga/path.h"
#include "taiga/taiga.h"
#include "base/xml.h"

#include "ui/dlg/dlg_anime_info.h"
#include "ui/dlg/dlg_anime_list.h"
#include "ui/dlg/dlg_season.h"

anime::Database AnimeDatabase;
anime::ImageDatabase ImageDatabase;

namespace anime {

bool Database::LoadDatabase() {
  xml_document document;
  wstring path = taiga::GetPath(taiga::kPathDatabaseAnime);
  unsigned int options = pugi::parse_default & ~pugi::parse_eol;
  xml_parse_result parse_result = document.load_file(path.c_str(), options);

  if (parse_result.status != pugi::status_ok &&
      parse_result.status != pugi::status_file_not_found) {
    return false;
  }

  xml_node animedb_node = document.child(L"animedb");

  foreach_xmlnode_(node, animedb_node, L"anime") {
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
    item.SetPopularity(XmlReadStrValue(node, L"popularity"));
    item.SetSynopsis(XmlReadStrValue(node, L"synopsis"));
    item.last_modified = _wtoi64(XmlReadStrValue(node, L"last_modified").c_str());
  }

  return true;
}

bool Database::SaveDatabase() {
  if (items.empty())
    return false;

  xml_document document;
  xml_node animedb_node = document.append_child(L"animedb");

  foreach_(it, items) {
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
    XML_WS(L"genres", Join(it->second.GetGenres(), L", "), pugi::node_pcdata);
    XML_WS(L"producers", Join(it->second.GetProducers(), L", "), pugi::node_pcdata);
    XML_WS(L"score", it->second.GetScore(), pugi::node_pcdata);
    XML_WS(L"popularity", it->second.GetPopularity(), pugi::node_pcdata);
    XML_WS(L"synopsis", it->second.GetSynopsis(), pugi::node_cdata);
    XML_WS(L"last_modified", ToWstr(it->second.last_modified), pugi::node_pcdata);
    #undef XML_WS
    #undef XML_WI
  }

  wstring path = taiga::GetPath(taiga::kPathDatabaseAnime);
  return XmlWriteDocumentToFile(document, path);
}

Item* Database::FindItem(int anime_id) {
  if (anime_id > ID_UNKNOWN) {
    auto it = items.find(anime_id);
    if (it != items.end())
      return &it->second;
  }

  return nullptr;
}

Item* Database::FindSequel(int anime_id) {
  int sequel_id = ID_UNKNOWN;

  switch (anime_id) {
    #define SEQUEL_ID(p, s) case p: sequel_id = s; break;
    // Gintama -> Gintama'
    SEQUEL_ID(918, 9969);
    // Tegami Bachi -> Tegami Bachi Reverse
    SEQUEL_ID(6444, 8311);
    // Fate/Zero -> Fate/Zero 2nd Season
    SEQUEL_ID(10087, 11741);
    // Towa no Qwon
    SEQUEL_ID(10294, 10713);
    SEQUEL_ID(10713, 10714);
    SEQUEL_ID(10714, 10715);
    SEQUEL_ID(10715, 10716);
    SEQUEL_ID(10716, 10717);
    #undef SEQUEL_ID
  }

  return FindItem(sequel_id);
}

int Database::GetItemCount(int status, bool check_history) {
  int count = 0;

  // Get current count
  foreach_(it, items)
    if (it->second.GetMyStatus(false) == status)
      count++;

  // Search queued items for status changes
  if (check_history) {
    foreach_(it, History.queue.items) {
      if (it->status) {
        if (it->mode == taiga::kHttpServiceAddLibraryEntry)
          continue;  // counted already
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
    if (new_item.GetEpisodeCount() > -1)
      item->SetEpisodeCount(new_item.GetEpisodeCount());
    if (new_item.GetAiringStatus(false) > 0)
      item->SetAiringStatus(new_item.GetAiringStatus());
    if (!new_item.GetTitle().empty())
      item->SetTitle(new_item.GetTitle());
    if (!new_item.GetEnglishTitle(false).empty())
      item->SetEnglishTitle(new_item.GetEnglishTitle());
    if (!new_item.GetSynonyms().empty())
      item->SetSynonyms(new_item.GetSynonyms());
    if (IsValidDate(new_item.GetDate(DATE_START)))
      item->SetDate(DATE_START, new_item.GetDate(DATE_START));
    if (IsValidDate(new_item.GetDate(DATE_END)))
      item->SetDate(DATE_END, new_item.GetDate(DATE_END));
    if (!new_item.GetImageUrl().empty())
      item->SetImageUrl(new_item.GetImageUrl());
    if (!new_item.GetGenres().empty())
      item->SetGenres(new_item.GetGenres());
    if (!new_item.GetPopularity().empty())
      item->SetPopularity(new_item.GetPopularity());
    if (!new_item.GetProducers().empty())
      item->SetProducers(new_item.GetProducers());
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
  ClearUserData();

  if (Settings.Account.MAL.user.empty())
    return false;

  xml_document document;
  wstring path = taiga::GetPath(taiga::kPathUserLibrary);
  xml_parse_result parse_result = document.load_file(path.c_str());

  if (parse_result.status != pugi::status_ok &&
      parse_result.status != pugi::status_file_not_found) {
    MessageBox(nullptr, L"Could not read anime list.", path.c_str(),
               MB_OK | MB_ICONERROR);
    return false;
  }

  xml_node node_myanimelist = document.child(L"myanimelist");

  foreach_xmlnode_(node, node_myanimelist, L"anime") {
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

  xml_document document;
  xml_node myanimelist_node = document.append_child(L"myanimelist");

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

  wstring path = taiga::GetPath(taiga::kPathUserLibrary);
  return XmlWriteDocumentToFile(document, path);
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
      if (anime_item->GetAiringStatus() != kFinishedAiring) {
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

    if (!SeasonDatabase.items.empty())
      if (std::find(SeasonDatabase.items.begin(), SeasonDatabase.items.end(), 
                    anime_id) != SeasonDatabase.items.end())
        if (SeasonDialog.IsVisible())
          erase = false;

    if (erase)
      items_.erase(anime_id);
  }
}

base::Image* ImageDatabase::GetImage(int anime_id) {
  if (items_.find(anime_id) != items_.end())
    if (items_[anime_id].data > 0)
      return &items_[anime_id];
  return nullptr;
}

} // namespace anime