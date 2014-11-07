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

#include "base/file.h"
#include "base/foreach.h"
#include "base/log.h"
#include "base/string.h"
#include "base/version.h"
#include "base/xml.h"
#include "library/anime.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "library/history.h"
#include "sync/manager.h"
#include "sync/myanimelist_util.h"
#include "sync/service.h"
#include "taiga/http.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "track/recognition.h"
#include "ui/dlg/dlg_anime_list.h"
#include "ui/ui.h"

anime::Database AnimeDatabase;

namespace anime {

bool Database::LoadDatabase() {
  xml_document document;
  std::wstring path = taiga::GetPath(taiga::kPathDatabaseAnime);
  unsigned int options = pugi::parse_default & ~pugi::parse_eol;
  xml_parse_result parse_result = document.load_file(path.c_str(), options);

  if (parse_result.status != pugi::status_ok)
    return false;

  xml_node meta_node = document.child(L"meta");
  std::wstring meta_version = XmlReadStrValue(meta_node, L"version");

  if (!meta_version.empty()) {
    xml_node database_node = document.child(L"database");
    ReadDatabaseNode(database_node);
    HandleCompatibility(meta_version);
  } else {
    LOG(LevelWarning, L"Reading database in compatibility mode");
    ReadDatabaseInCompatibilityMode(document);
  }

  return true;
}

void Database::ReadDatabaseNode(xml_node& database_node) {
  foreach_xmlnode_(node, database_node, L"anime") {
    std::map<enum_t, std::wstring> id_map;

    foreach_xmlnode_(id_node, node, L"id") {
      std::wstring id = id_node.child_value();
      std::wstring name = id_node.attribute(L"name").as_string();
      enum_t service_id = ServiceManager.GetServiceIdByName(name);
      id_map[service_id] = id;
    }

    enum_t source = sync::kTaiga;
    auto service = ServiceManager.service(XmlReadStrValue(node, L"source"));
    if (service)
      source = service->id();

    if (source == sync::kTaiga) {
      auto current_service_id = taiga::GetCurrentServiceId();
      if (id_map.find(current_service_id) != id_map.end()) {
        source = current_service_id;
        LOG(LevelWarning, L"Fixed source for ID: " + id_map[source]);
      } else {
        LOG(LevelError, L"Invalid source for ID: " + id_map[sync::kTaiga]);
        continue;
      }
    }

    Item& item = items[ToInt(id_map[sync::kTaiga])];  // Creates the item if it doesn't exist

    foreach_(it, id_map)
      item.SetId(it->second, it->first);

    std::vector<std::wstring> synonyms;
    XmlReadChildNodes(node, synonyms, L"synonym");

    item.SetSource(source);
    item.SetSlug(XmlReadStrValue(node, L"slug"));

    item.SetTitle(XmlReadStrValue(node, L"title"));
    item.SetEnglishTitle(XmlReadStrValue(node, L"english"));
    item.SetSynonyms(synonyms);
    item.SetType(XmlReadIntValue(node, L"type"));
    item.SetAiringStatus(XmlReadIntValue(node, L"status"));
    item.SetEpisodeCount(XmlReadIntValue(node, L"episode_count"));
    item.SetEpisodeLength(XmlReadIntValue(node, L"episode_length"));
    item.SetDateStart(Date(XmlReadStrValue(node, L"date_start")));
    item.SetDateEnd(Date(XmlReadStrValue(node, L"date_end")));
    item.SetImageUrl(XmlReadStrValue(node, L"image"));
    item.SetAgeRating(XmlReadIntValue(node, L"age_rating"));
    item.SetGenres(XmlReadStrValue(node, L"genres"));
    item.SetProducers(XmlReadStrValue(node, L"producers"));
    item.SetScore(ToDouble(XmlReadStrValue(node, L"score")));
    item.SetPopularity(XmlReadIntValue(node, L"popularity"));
    item.SetSynopsis(XmlReadStrValue(node, L"synopsis"));
    item.SetLastModified(_wtoi64(XmlReadStrValue(node, L"modified").c_str()));
  }
}

bool Database::SaveDatabase() {
  if (items.empty())
    return false;

  xml_document document;

  xml_node meta_node = document.append_child(L"meta");
  XmlWriteStrValue(meta_node, L"version", std::wstring(Taiga.version).c_str());

  xml_node database_node = document.append_child(L"database");
  WriteDatabaseNode(database_node);

  std::wstring path = taiga::GetPath(taiga::kPathDatabaseAnime);
  return XmlWriteDocumentToFile(document, path);
}

void Database::WriteDatabaseNode(xml_node& database_node) {
  foreach_(it, items) {
    xml_node anime_node = database_node.append_child(L"anime");

    for (int i = 0; i <= sync::kLastService; i++) {
      std::wstring id = it->second.GetId(i);
      if (!id.empty()) {
        xml_node child = anime_node.append_child(L"id");
        std::wstring name = ServiceManager.GetServiceNameById(static_cast<sync::ServiceId>(i));
        child.append_attribute(L"name") = name.c_str();
        child.append_child(pugi::node_pcdata).set_value(id.c_str());
      }
    }

    std::wstring source = ServiceManager.GetServiceNameById(
        static_cast<sync::ServiceId>(it->second.GetSource()));

    #define XML_WC(n, v, t) \
      if (!v.empty()) XmlWriteChildNodes(anime_node, v, n, t)
    #define XML_WD(n, v) \
      if (v) XmlWriteStrValue(anime_node, n, std::wstring(v).c_str())
    #define XML_WI(n, v) \
      if (v > 0) XmlWriteIntValue(anime_node, n, v)
    #define XML_WS(n, v, t) \
      if (!v.empty()) XmlWriteStrValue(anime_node, n, v.c_str(), t)
    #define XML_WF(n, v, t) \
      if (v > 0.0) XmlWriteStrValue(anime_node, n, ToWstr(v).c_str(), t)
    XML_WS(L"source", source, pugi::node_pcdata);
    XML_WS(L"slug", it->second.GetSlug(), pugi::node_pcdata);
    XML_WS(L"title", it->second.GetTitle(), pugi::node_cdata);
    XML_WS(L"english", it->second.GetEnglishTitle(), pugi::node_cdata);
    XML_WC(L"synonym", it->second.GetSynonyms(), pugi::node_cdata);
    XML_WI(L"type", it->second.GetType());
    XML_WI(L"status", it->second.GetAiringStatus());
    XML_WI(L"episode_count", it->second.GetEpisodeCount());
    XML_WI(L"episode_length", it->second.GetEpisodeLength());
    XML_WD(L"date_start", it->second.GetDateStart());
    XML_WD(L"date_end", it->second.GetDateEnd());
    XML_WS(L"image", it->second.GetImageUrl(), pugi::node_pcdata);
    XML_WI(L"age_rating", it->second.GetAgeRating());
    XML_WS(L"genres", Join(it->second.GetGenres(), L", "), pugi::node_pcdata);
    XML_WS(L"producers", Join(it->second.GetProducers(), L", "), pugi::node_pcdata);
    XML_WF(L"score", it->second.GetScore(), pugi::node_pcdata);
    XML_WI(L"popularity", it->second.GetPopularity());
    XML_WS(L"synopsis", it->second.GetSynopsis(), pugi::node_cdata);
    XML_WS(L"modified", ToWstr(it->second.GetLastModified()), pugi::node_pcdata);
    #undef XML_WF
    #undef XML_WS
    #undef XML_WI
    #undef XML_WD
    #undef XML_WC
  }
}

////////////////////////////////////////////////////////////////////////////////

Item* Database::FindItem(int id) {
  if (IsValidId(id)) {
    auto it = items.find(id);
    if (it != items.end())
      return &it->second;
  }

  return nullptr;
}

Item* Database::FindItem(const std::wstring& id, enum_t service) {
  if (!id.empty())
    foreach_(it, items)
      if (id == it->second.GetId(service))
        return &it->second;

  return nullptr;
}

Item* Database::FindSequel(int anime_id) {
  if (taiga::GetCurrentServiceId() != sync::kMyAnimeList)
    return nullptr;

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
  Item* item = nullptr;

  for (enum_t i = sync::kTaiga; i <= sync::kLastService; i++) {
    item = FindItem(new_item.GetId(i), i);
    if (item)
      break;
  }

  if (!item) {
    auto source = new_item.GetSource();

    if (source == sync::kTaiga) {
      LOG(LevelError, L"Invalid source for ID: " + new_item.GetId(source));
      return ID_UNKNOWN;
    }

    int id = ToInt(new_item.GetId(source));

    // Add a new item
    item = &items[id];
    item->SetId(ToWstr(id), sync::kTaiga);
  }

  // Update series information if new information is, well, new.
  if (!item->GetLastModified() ||
      new_item.GetLastModified() >= item->GetLastModified()) {
    item->SetLastModified(new_item.GetLastModified());

    for (enum_t i = sync::kFirstService; i <= sync::kLastService; i++)
      if (!new_item.GetId(i).empty())
        item->SetId(new_item.GetId(i), i);

    if (new_item.GetSource() != sync::kTaiga)
      item->SetSource(new_item.GetSource());

    if (new_item.GetType() != kUnknownType)
      item->SetType(new_item.GetType());
    if (new_item.GetEpisodeCount() != kUnknownEpisodeCount)
      item->SetEpisodeCount(new_item.GetEpisodeCount());
    if (new_item.GetEpisodeLength() != kUnknownEpisodeLength)
      item->SetEpisodeLength(new_item.GetEpisodeLength());
    if (new_item.GetAiringStatus(false) != kUnknownStatus)
      item->SetAiringStatus(new_item.GetAiringStatus());
    if (!new_item.GetSlug().empty())
      item->SetSlug(new_item.GetSlug());
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
    if (new_item.GetAgeRating() != kUnknownAgeRating)
      item->SetAgeRating(new_item.GetAgeRating());
    if (!new_item.GetGenres().empty())
      item->SetGenres(new_item.GetGenres());
    if (new_item.GetPopularity() > 0)
      item->SetPopularity(new_item.GetPopularity());
    if (!new_item.GetProducers().empty())
      item->SetProducers(new_item.GetProducers());
    if (new_item.GetScore() != kUnknownScore)
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

    if (!item->GetNextEpisodePath().empty() && 
        item->GetMyLastWatchedEpisode() != new_item.GetMyLastWatchedEpisode()) {
      // Next episode path is no longer valid
      item->SetNextEpisodePath(L"");
    }

    item->SetMyLastWatchedEpisode(new_item.GetMyLastWatchedEpisode(false));
    item->SetMyScore(new_item.GetMyScore(false));
    item->SetMyStatus(new_item.GetMyStatus(false));
    item->SetMyRewatchedTimes(new_item.GetMyRewatchedTimes());
    item->SetMyRewatching(new_item.GetMyRewatching(false));
    item->SetMyRewatchingEp(new_item.GetMyRewatchingEp());
    item->SetMyDateStart(new_item.GetMyDateStart());
    item->SetMyDateEnd(new_item.GetMyDateEnd());
    item->SetMyLastUpdated(new_item.GetMyLastUpdated());
    item->SetMyTags(new_item.GetMyTags(false));
  }

  return item->GetId();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool Database::LoadList() {
  ClearUserData();

  if (taiga::GetCurrentUsername().empty())
    return false;

  xml_document document;
  std::wstring path = taiga::GetPath(taiga::kPathUserLibrary);
  xml_parse_result parse_result = document.load_file(path.c_str());

  if (parse_result.status != pugi::status_ok) {
    if (parse_result.status == pugi::status_file_not_found) {
      return CheckOldUserDirectory();
    } else {
      ui::DisplayErrorMessage(L"Could not read anime list.", path.c_str());
      return false;
    }
  }

  xml_node meta_node = document.child(L"meta");
  std::wstring meta_version = XmlReadStrValue(meta_node, L"version");

  if (!meta_version.empty()) {
    xml_node node_database = document.child(L"database");
    ReadDatabaseNode(node_database);

    xml_node node_library = document.child(L"library");
    foreach_xmlnode_(node, node_library, L"anime") {
      Item anime_item;
      anime_item.SetId(XmlReadStrValue(node, L"id"), sync::kTaiga);
      anime_item.SetSource(sync::kTaiga);

      anime_item.AddtoUserList();
      anime_item.SetMyLastWatchedEpisode(XmlReadIntValue(node, L"progress"));
      anime_item.SetMyDateStart(XmlReadStrValue(node, L"date_start"));
      anime_item.SetMyDateEnd(XmlReadStrValue(node, L"date_end"));
      anime_item.SetMyScore(XmlReadIntValue(node, L"score"));
      anime_item.SetMyStatus(XmlReadIntValue(node, L"status"));
      anime_item.SetMyRewatchedTimes(XmlReadIntValue(node, L"rewatched_times"));
      anime_item.SetMyRewatching(XmlReadIntValue(node, L"rewatching"));
      anime_item.SetMyRewatchingEp(XmlReadIntValue(node, L"rewatching_ep"));
      anime_item.SetMyTags(XmlReadStrValue(node, L"tags"));
      anime_item.SetMyLastUpdated(XmlReadStrValue(node, L"last_updated"));

      UpdateItem(anime_item);
    }

  } else {
    LOG(LevelWarning, L"Reading list in compatibility mode");
    ReadListInCompatibilityMode(document);
  }

  return true;
}

bool Database::SaveList(bool include_database) {
  if (items.empty())
    return false;

  xml_document document;

  xml_node meta_node = document.append_child(L"meta");
  XmlWriteStrValue(meta_node, L"version", L"1.1");

  if (include_database) {
    xml_node node_database = document.append_child(L"database");
    WriteDatabaseNode(node_database);
  }

  xml_node node_library = document.append_child(L"library");

  foreach_(it, items) {
    Item* item = &it->second;
    if (item->IsInList()) {
      xml_node node = node_library.append_child(L"anime");
      XmlWriteIntValue(node, L"id", item->GetId());
      XmlWriteIntValue(node, L"progress", item->GetMyLastWatchedEpisode(false));
      XmlWriteStrValue(node, L"date_start", std::wstring(item->GetMyDateStart()).c_str());
      XmlWriteStrValue(node, L"date_end", std::wstring(item->GetMyDateEnd()).c_str());
      XmlWriteIntValue(node, L"score", item->GetMyScore(false));
      XmlWriteIntValue(node, L"status", item->GetMyStatus(false));
      XmlWriteIntValue(node, L"rewatched_times", item->GetMyRewatchedTimes());
      XmlWriteIntValue(node, L"rewatching", item->GetMyRewatching(false));
      XmlWriteIntValue(node, L"rewatching_ep", item->GetMyRewatchingEp());
      XmlWriteStrValue(node, L"tags", item->GetMyTags(false).c_str());
      XmlWriteStrValue(node, L"last_updated", item->GetMyLastUpdated().c_str());
    }
  }

  std::wstring path = taiga::GetPath(taiga::kPathUserLibrary);
  return XmlWriteDocumentToFile(document, path);
}

////////////////////////////////////////////////////////////////////////////////

int Database::GetItemCount(int status, bool check_history) {
  // Get current count
  int count = std::count_if(items.begin(), items.end(),
      [&](const std::pair<int, Item>& it) {
        if (it.second.GetMyRewatching())
          return status == kWatching;
        return it.second.GetMyStatus(false) == status;
      });

  // Search queued items for status changes
  if (check_history) {
    foreach_(it, History.queue.items) {
      if (it->status || it->mode == taiga::kHttpServiceDeleteLibraryEntry) {
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

void Database::AddToList(int anime_id, int status) {
  auto anime_item = FindItem(anime_id);

  if (!anime_item)
    return;

  anime_item->AddtoUserList();

  HistoryItem history_item;
  history_item.anime_id = anime_id;
  history_item.status = status;
  if (status == anime::kCompleted) {
    history_item.episode = anime_item->GetEpisodeCount();
    if (anime_item->GetEpisodeCount() == 1)
      history_item.date_start = GetDate();
    history_item.date_finish = GetDate();
  }
  history_item.mode = taiga::kHttpServiceAddLibraryEntry;
  History.queue.Add(history_item);

  SaveList();

  ui::OnLibraryEntryAdd(anime_id);

  if (CurrentEpisode.anime_id == anime::ID_NOTINLIST)
    CurrentEpisode.Set(anime::ID_UNKNOWN);
}

void Database::ClearUserData() {
  ui::DlgAnimeList.SetCurrentId(ID_UNKNOWN);

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

  if (CurrentEpisode.anime_id == anime_id)
    CurrentEpisode.Set(anime::ID_NOTINLIST);

  return true;
}

void Database::UpdateItem(const HistoryItem& history_item) {
  auto anime_item = FindItem(history_item.anime_id);

  if (!anime_item)
    return;

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
  // Edit rewatching status
  if (history_item.enable_rewatching) {
    anime_item->SetMyRewatching(*history_item.enable_rewatching);
  }
  if (history_item.rewatched_times) {
    anime_item->SetMyRewatchedTimes(*history_item.rewatched_times);
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
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool Database::CheckOldUserDirectory() {
  std::wstring path = taiga::GetPath(taiga::kPathUser) +
                      taiga::GetCurrentUsername();

  if (FolderExists(path)) {
    LOG(LevelWarning, L"Moving old user directory to its new place");
    auto service_name = ServiceManager.GetServiceNameById(sync::kMyAnimeList);
    std::wstring new_path = path + L"@" + service_name;
    if (MoveFileEx(path.c_str(), new_path.c_str(), 0) != 0) {
      return LoadList();
    }
  }

  return true;
}

void Database::ClearInvalidValues(Item& item) {
  const std::wstring invalid_title = L"S"L"S"L"J"L"Master";
  const std::wstring invalid_description =
      L"Prepare yourself for another 9001 action-packed episodes";

  if (item.GetEnglishTitle() == invalid_title)
    item.SetEnglishTitle(L"");

  auto synonyms = item.GetSynonyms();
  if (!synonyms.empty()) {
    foreach_(synonym, synonyms) {
      if (*synonym == invalid_title)
        *synonym = L"";
    }
    RemoveEmptyStrings(synonyms);
    item.SetSynonyms(synonyms);
  }

  if (StartsWith(item.GetSynopsis(), invalid_description))
    item.SetSynopsis(L"");
}

void Database::HandleCompatibility(const std::wstring& meta_version) {
  base::SemanticVersion version = meta_version;

  if (version <= base::SemanticVersion(L"1.1.7")) {
    LOG(LevelWarning, L"Clearing invalid values");
    foreach_(item, items) {
      ClearInvalidValues(item->second);
    }
  }
}

void Database::ReadDatabaseInCompatibilityMode(xml_document& document) {
  xml_node animedb_node = document.child(L"animedb");

  foreach_xmlnode_(node, animedb_node, L"anime") {
    std::wstring id = XmlReadStrValue(node, L"series_animedb_id");
    Item& item = items[ToInt(id)];  // Creates the item if it doesn't exist
    item.SetId(id, sync::kTaiga);
    item.SetId(id, sync::kMyAnimeList);
    item.SetTitle(XmlReadStrValue(node, L"series_title"));
    item.SetEnglishTitle(XmlReadStrValue(node, L"series_english"));
    item.SetSynonyms(XmlReadStrValue(node, L"series_synonyms"));
    item.SetType(sync::myanimelist::TranslateSeriesTypeFrom(XmlReadIntValue(node, L"series_type")));
    item.SetEpisodeCount(XmlReadIntValue(node, L"series_episodes"));
    item.SetAiringStatus(sync::myanimelist::TranslateSeriesStatusFrom(XmlReadIntValue(node, L"series_status")));
    item.SetDateStart(Date(XmlReadStrValue(node, L"series_start")));
    item.SetDateEnd(Date(XmlReadStrValue(node, L"series_end")));
    item.SetImageUrl(XmlReadStrValue(node, L"series_image"));
    item.SetGenres(XmlReadStrValue(node, L"genres"));
    item.SetProducers(XmlReadStrValue(node, L"producers"));
    item.SetScore(ToDouble(XmlReadStrValue(node, L"score")));
    item.SetPopularity(XmlReadIntValue(node, L"popularity"));
    item.SetSynopsis(XmlReadStrValue(node, L"synopsis"));
    item.SetLastModified(_wtoi64(XmlReadStrValue(node, L"last_modified").c_str()));
  }
}

void Database::ReadListInCompatibilityMode(xml_document& document) {
  xml_node myanimelist = document.child(L"myanimelist");

  foreach_xmlnode_(node, myanimelist, L"anime") {
    Item anime_item;
    anime_item.SetId(XmlReadStrValue(node, L"series_animedb_id"), sync::kMyAnimeList);
    anime_item.SetSource(sync::kMyAnimeList);

    anime_item.SetTitle(XmlReadStrValue(node, L"series_title"));
    anime_item.SetSynonyms(XmlReadStrValue(node, L"series_synonyms"));
    anime_item.SetType(sync::myanimelist::TranslateSeriesTypeFrom(XmlReadIntValue(node, L"series_type")));
    anime_item.SetEpisodeCount(XmlReadIntValue(node, L"series_episodes"));
    anime_item.SetAiringStatus(sync::myanimelist::TranslateSeriesStatusFrom(XmlReadIntValue(node, L"series_status")));
    anime_item.SetDateStart(XmlReadStrValue(node, L"series_start"));
    anime_item.SetDateEnd(XmlReadStrValue(node, L"series_end"));
    anime_item.SetImageUrl(XmlReadStrValue(node, L"series_image"));
    anime_item.SetLastModified(0);

    anime_item.AddtoUserList();
    anime_item.SetMyLastWatchedEpisode(XmlReadIntValue(node, L"my_watched_episodes"));
    anime_item.SetMyDateStart(XmlReadStrValue(node, L"my_start_date"));
    anime_item.SetMyDateEnd(XmlReadStrValue(node, L"my_finish_date"));
    anime_item.SetMyScore(XmlReadIntValue(node, L"my_score"));
    anime_item.SetMyStatus(sync::myanimelist::TranslateMyStatusFrom(XmlReadIntValue(node, L"my_status")));
    anime_item.SetMyRewatching(XmlReadIntValue(node, L"my_rewatching"));
    anime_item.SetMyRewatchingEp(XmlReadIntValue(node, L"my_rewatching_ep"));
    anime_item.SetMyLastUpdated(XmlReadStrValue(node, L"my_last_updated"));
    anime_item.SetMyTags(XmlReadStrValue(node, L"my_tags"));

    UpdateItem(anime_item);
  }
}

}  // namespace anime