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

#include "base/foreach.h"
#include "base/logger.h"
#include "base/string.h"
#include "base/xml.h"
#include "library/anime.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "library/history.h"
#include "sync/manager.h"
#include "sync/service.h"
#include "taiga/http.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "track/recognition.h"
#include "ui/dlg/dlg_anime_list.h"
#include "ui/ui.h"

anime::Database AnimeDatabase;

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

  xml_node meta_node = document.child(L"meta");
  wstring meta_version = XmlReadStrValue(meta_node, L"version");

  xml_node database_node = document.child(L"database");

  foreach_xmlnode_(node, database_node, L"anime") {
    std::vector<std::wstring> ids;
    XmlReadChildNodes(node, ids, L"id");

    Item& item = items[ToInt(ids.at(0))];  // Creates the item if it doesn't exist

    for (size_t i = 0; i < ids.size(); i++)
      item.SetId(ids.at(i), i);

    item.SetSource(XmlReadIntValue(node, L"source"));

    item.SetTitle(XmlReadStrValue(node, L"title"));
    item.SetEnglishTitle(XmlReadStrValue(node, L"english"));
    item.SetSynonyms(XmlReadStrValue(node, L"synonyms"));
    item.SetType(XmlReadIntValue(node, L"type"));
    item.SetAiringStatus(XmlReadIntValue(node, L"status"));
    item.SetEpisodeCount(XmlReadIntValue(node, L"episode_count"));
    item.SetEpisodeLength(XmlReadIntValue(node, L"episode_length"));
    item.SetDateStart(Date(XmlReadStrValue(node, L"date_start")));
    item.SetDateEnd(Date(XmlReadStrValue(node, L"date_end")));
    item.SetImageUrl(XmlReadStrValue(node, L"image"));
    item.SetGenres(XmlReadStrValue(node, L"genres"));
    item.SetProducers(XmlReadStrValue(node, L"producers"));
    item.SetScore(XmlReadStrValue(node, L"score"));
    item.SetPopularity(XmlReadStrValue(node, L"popularity"));
    item.SetSynopsis(XmlReadStrValue(node, L"synopsis"));
    item.last_modified = _wtoi64(XmlReadStrValue(node, L"modified").c_str());
  }

  return true;
}

bool Database::SaveDatabase() {
  if (items.empty())
    return false;

  xml_document document;

  xml_node meta_node = document.append_child(L"meta");
  XmlWriteStrValue(meta_node, L"version", L"1.1");

  xml_node database_node = document.append_child(L"database");

  foreach_(it, items) {
    xml_node anime_node = database_node.append_child(L"anime");

    for (int i = 0; i <= sync::kHerro; i++)
      XmlWriteStrValue(anime_node, L"id", it->second.GetId(i).c_str());

    XmlWriteIntValue(anime_node, L"source", it->second.GetSource());

    #define XML_WD(n, v) \
      if (v) XmlWriteStrValue(anime_node, n, wstring(v).c_str())
    #define XML_WI(n, v) \
      if (v > 0) XmlWriteIntValue(anime_node, n, v)
    #define XML_WS(n, v, t) \
      if (!v.empty()) XmlWriteStrValue(anime_node, n, v.c_str(), t)
    XML_WS(L"title", it->second.GetTitle(), pugi::node_cdata);
    XML_WS(L"english", it->second.GetEnglishTitle(), pugi::node_cdata);
    XML_WS(L"synonyms", Join(it->second.GetSynonyms(), L"; "), pugi::node_cdata);
    XML_WI(L"type", it->second.GetType());
    XML_WI(L"status", it->second.GetAiringStatus());
    XML_WI(L"episode_count", it->second.GetEpisodeCount());
    XML_WI(L"episode_length", it->second.GetEpisodeLength());
    XML_WD(L"date_start", it->second.GetDateStart());
    XML_WD(L"date_end", it->second.GetDateEnd());
    XML_WS(L"image", it->second.GetImageUrl(), pugi::node_pcdata);
    XML_WS(L"genres", Join(it->second.GetGenres(), L", "), pugi::node_pcdata);
    XML_WS(L"producers", Join(it->second.GetProducers(), L", "), pugi::node_pcdata);
    XML_WS(L"score", it->second.GetScore(), pugi::node_pcdata);
    XML_WS(L"popularity", it->second.GetPopularity(), pugi::node_pcdata);
    XML_WS(L"synopsis", it->second.GetSynopsis(), pugi::node_cdata);
    XML_WS(L"modified", ToWstr(it->second.last_modified), pugi::node_pcdata);
    #undef XML_WS
    #undef XML_WI
    #undef XML_WD
  }

  wstring path = taiga::GetPath(taiga::kPathDatabaseAnime);
  return XmlWriteDocumentToFile(document, path);
}

////////////////////////////////////////////////////////////////////////////////

Item* Database::FindItem(int id) {
  if (id > ID_UNKNOWN) {
    auto it = items.find(id);
    if (it != items.end())
      return &it->second;
  }

  return nullptr;
}

Item* Database::FindItem(const std::wstring& id, enum_t service) {
  foreach_(it, items)
    if (id == it->second.GetId(service))
      return &it->second;

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

////////////////////////////////////////////////////////////////////////////////

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

int Database::UpdateItem(const Item& new_item) {
  critical_section_.Enter();

  enum_t source = new_item.GetSource();
  Item* item = FindItem(new_item.GetId(source), source);
  if (!item) {
    /*
    auto service = ServiceManager.service(Settings[taiga::kSync_ActiveService]);
    if (new_item.GetSource() != service->id()) {
      // TODO: Try to find the same item by similarity
    }
    */

    // Generate a new ID
    int id = 1;
    while (FindItem(id))
      ++id;
    // Add a new item
    item = &items[id];
    item->SetId(ToWstr(id), 0);
  }

  // Update series information if new information is, well, new.
  if (!item->last_modified || new_item.last_modified > item->last_modified) {
    item->SetId(new_item.GetId(source), source);
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
    if (IsValidDate(new_item.GetDateStart()))
      item->SetDateStart(new_item.GetDateStart());
    if (IsValidDate(new_item.GetDateEnd()))
      item->SetDateEnd(new_item.GetDateEnd());
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
      Meow.UpdateCleanTitles(item->GetId());
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
    item->SetMyDateStart(new_item.GetMyDateStart());
    item->SetMyDateEnd(new_item.GetMyDateEnd());
    item->SetMyLastUpdated(new_item.GetMyLastUpdated());
    item->SetMyTags(new_item.GetMyTags(false));
  }

  critical_section_.Leave();

  return item->GetId();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool Database::LoadList() {
  ClearUserData();

  if (taiga::GetCurrentUsername().empty())
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

  xml_node meta_node = document.child(L"meta");
  wstring meta_version = XmlReadStrValue(meta_node, L"version");

  xml_node node_library = document.child(L"library");

  foreach_xmlnode_(node, node_library, L"anime") {
    Item anime_item;
    anime_item.SetId(XmlReadStrValue(node, L"id"), 0);

    anime_item.AddtoUserList();
    anime_item.SetMyLastWatchedEpisode(XmlReadIntValue(node, L"progress"));
    anime_item.SetMyDateStart(XmlReadStrValue(node, L"date_start"));
    anime_item.SetMyDateEnd(XmlReadStrValue(node, L"date_end"));
    anime_item.SetMyScore(XmlReadIntValue(node, L"score"));
    anime_item.SetMyStatus(XmlReadIntValue(node, L"status"));
    anime_item.SetMyRewatching(XmlReadIntValue(node, L"rewatching"));
    anime_item.SetMyRewatchingEp(XmlReadIntValue(node, L"rewatching_ep"));
    anime_item.SetMyTags(XmlReadStrValue(node, L"tags"));
    anime_item.SetMyLastUpdated(XmlReadStrValue(node, L"last_updated"));

    UpdateItem(anime_item);
  }

  return true;
}

bool Database::SaveList() {
  if (items.empty())
    return false;

  xml_document document;

  xml_node meta_node = document.append_child(L"meta");
  XmlWriteStrValue(meta_node, L"version", L"1.1");

  xml_node node_library = document.append_child(L"library");

  foreach_(it, items) {
    Item* item = &it->second;
    if (item->IsInList()) {
      xml_node node = node_library.append_child(L"anime");
      XmlWriteIntValue(node, L"id", item->GetId());
      XmlWriteIntValue(node, L"progress", item->GetMyLastWatchedEpisode(false));
      XmlWriteStrValue(node, L"date_start", wstring(item->GetMyDateStart()).c_str());
      XmlWriteStrValue(node, L"date_end", wstring(item->GetMyDateEnd()).c_str());
      XmlWriteIntValue(node, L"score", item->GetMyScore(false));
      XmlWriteIntValue(node, L"status", item->GetMyStatus(false));
      XmlWriteIntValue(node, L"rewatching", item->GetMyRewatching(false));
      XmlWriteIntValue(node, L"rewatching_ep", item->GetMyRewatchingEp());
      XmlWriteStrValue(node, L"tags", item->GetMyTags(false).c_str());
      XmlWriteStrValue(node, L"last_updated", item->GetMyLastUpdated().c_str());
    }
  }

  wstring path = taiga::GetPath(taiga::kPathUserLibrary);
  return XmlWriteDocumentToFile(document, path);
}

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

void Database::ClearUserData() {
  AnimeListDialog.SetCurrentId(ID_UNKNOWN);
  
  foreach_(it, items)
    it->second.RemoveFromUserList();
}

bool Database::DeleteListItem(int anime_id) {
  auto anime_item = FindItem(anime_id);

  if (!anime_item)
    return false;
  if (!anime_item->IsInList())
    return false;

  anime_item->RemoveFromUserList();

  ui::ChangeStatusText(L"Item deleted. (" + anime_item->GetTitle() + L")");
  ui::OnLibraryEntryDelete(anime_item->GetId());

  return true;
}

void Database::UpdateItem(const HistoryItem& history_item) {
  auto anime_item = FindItem(history_item.anime_id);

  if (!anime_item)
    return;

  critical_section_.Enter();

  // Edit episode
  if (history_item.episode) {
    anime_item->SetMyLastWatchedEpisode(*history_item.episode);
  }
  // Edit score
  if (history_item.score) {
    anime_item->SetMyScore(*history_item.score);
  }
  // Edit status
  if (history_item.status) {
    anime_item->SetMyStatus(*history_item.status);
  }
  // Edit re-watching status
  if (history_item.enable_rewatching) {
    anime_item->SetMyRewatching(*history_item.enable_rewatching);
  }
  // Edit tags
  if (history_item.tags) {
    anime_item->SetMyTags(*history_item.tags);
  }
  // Edit dates
  if (history_item.date_start) {
    anime_item->SetMyDateStart(*history_item.date_start);
  }
  if (history_item.date_finish) {
    anime_item->SetMyDateEnd(*history_item.date_finish);
  }
  // Delete
  if (history_item.mode == taiga::kHttpServiceDeleteLibraryEntry) {
    DeleteListItem(anime_item->GetId());
  }

  SaveList();

  History.queue.Remove();
  History.queue.Check(false);

  ui::OnLibraryEntryChange(history_item.anime_id);

  critical_section_.Leave();
}

}  // namespace anime