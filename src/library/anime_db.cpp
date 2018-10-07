/*
** Taiga
** Copyright (C) 2010-2018, Eren Okka
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

#include <algorithm>

#include "base/file.h"
#include "base/log.h"
#include "base/string.h"
#include "base/xml.h"
#include "library/anime.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "library/discover.h"
#include "library/history.h"
#include "sync/manager.h"
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
  std::wstring path = taiga::GetPath(taiga::Path::DatabaseAnime);
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
    LOGW(L"Reading database in compatibility mode");
    ReadDatabaseInCompatibilityMode(document);
  }

  return true;
}

void Database::ReadDatabaseNode(xml_node& database_node) {
  foreach_xmlnode_(node, database_node, L"anime") {
    std::map<enum_t, std::wstring> id_map;

    foreach_xmlnode_(id_node, node, L"id") {
      const std::wstring name = id_node.attribute(L"name").as_string();
      if (name == L"hummingbird") {
        id_map[sync::kKitsu] = id_node.child_value();
      } else {
        enum_t service_id = ServiceManager.GetServiceIdByName(name);
        id_map[service_id] = id_node.child_value();
      }
    }

    enum_t source = sync::kTaiga;
    const std::wstring source_name = XmlReadStrValue(node, L"source");
    if (source_name == L"hummingbird") {
      source = sync::kKitsu;
    } else {
      auto service = ServiceManager.service(source_name);
      if (service)
        source = service->id();
    }

    if (source == sync::kTaiga) {
      auto current_service_id = taiga::GetCurrentServiceId();
      if (id_map.find(current_service_id) != id_map.end()) {
        source = current_service_id;
        LOGW(L"Fixed source for ID: {}", id_map[source]);
      } else {
        LOGE(L"Invalid source for ID: {}", id_map[sync::kTaiga]);
        continue;
      }
    }

    int id = ToInt(id_map[sync::kTaiga]);
    Item& item = items[id];  // Creates the item if it doesn't exist

    for (const auto& pair : id_map)
      item.SetId(pair.second, pair.first);

    item.SetSource(source);
    item.SetTitle(XmlReadStrValue(node, L"title"));
    item.SetType(XmlReadIntValue(node, L"type"));
    item.SetAiringStatus(XmlReadIntValue(node, L"status"));
    item.SetAgeRating(XmlReadIntValue(node, L"age_rating"));
    item.SetGenres(XmlReadStrValue(node, L"genres"));
    item.SetProducers(XmlReadStrValue(node, L"producers"));
    item.SetSynopsis(XmlReadStrValue(node, L"synopsis"));
    item.SetLastModified(ToTime(XmlReadStrValue(node, L"modified")));

    // This ordering results in less reallocations
    item.SetEnglishTitle(XmlReadStrValue(node, L"english"));  // alternative
    item.SetJapaneseTitle(XmlReadStrValue(node, L"japanese"));  // alternative
    foreach_xmlnode_(child_node, node, L"synonym")
      item.InsertSynonym(child_node.child_value());  // alternative
    item.SetPopularity(XmlReadIntValue(node, L"popularity"));  // community(1)
    item.SetScore(ToDouble(XmlReadStrValue(node, L"score")));  // community(0)
    item.SetDateEnd(Date(XmlReadStrValue(node, L"date_end")));      // date(1)
    item.SetDateStart(Date(XmlReadStrValue(node, L"date_start")));  // date(0)
    item.SetEpisodeLength(XmlReadIntValue(node, L"episode_length"));  // extent(1)
    item.SetEpisodeCount(XmlReadIntValue(node, L"episode_count"));    // extent(0)
    item.SetSlug(XmlReadStrValue(node, L"slug"));       // resource(1)
    item.SetImageUrl(XmlReadStrValue(node, L"image"));  // resource(0)
  }
}

bool Database::SaveDatabase() {
  xml_document document;

  xml_node meta_node = document.append_child(L"meta");
  XmlWriteStrValue(meta_node, L"version", StrToWstr(Taiga.version.to_string()).c_str());

  xml_node database_node = document.append_child(L"database");
  WriteDatabaseNode(database_node);

  std::wstring path = taiga::GetPath(taiga::Path::DatabaseAnime);
  return XmlWriteDocumentToFile(document, path);
}

void Database::WriteDatabaseNode(xml_node& database_node) {
  for (const auto& pair : items) {
    xml_node anime_node = database_node.append_child(L"anime");

    for (int i = 0; i <= sync::kLastService; i++) {
      std::wstring id = pair.second.GetId(i);
      if (!id.empty()) {
        xml_node child = anime_node.append_child(L"id");
        std::wstring name = ServiceManager.GetServiceNameById(static_cast<sync::ServiceId>(i));
        child.append_attribute(L"name") = name.c_str();
        child.append_child(pugi::node_pcdata).set_value(id.c_str());
      }
    }

    std::wstring source = ServiceManager.GetServiceNameById(
        static_cast<sync::ServiceId>(pair.second.GetSource()));

    #define XML_WC(n, v, t) \
      if (!v.empty()) XmlWriteChildNodes(anime_node, v, n, t)
    #define XML_WD(n, v) \
      if (v) XmlWriteStrValue(anime_node, n, v.to_string().c_str())
    #define XML_WI(n, v) \
      if (v > 0) XmlWriteIntValue(anime_node, n, v)
    #define XML_WS(n, v, t) \
      if (!v.empty()) XmlWriteStrValue(anime_node, n, v.c_str(), t)
    #define XML_WF(n, v, t) \
      if (v > 0.0) XmlWriteStrValue(anime_node, n, ToWstr(v).c_str(), t)
    XML_WS(L"source", source, pugi::node_pcdata);
    XML_WS(L"slug", pair.second.GetSlug(), pugi::node_pcdata);
    XML_WS(L"title", pair.second.GetTitle(), pugi::node_cdata);
    XML_WS(L"english", pair.second.GetEnglishTitle(), pugi::node_cdata);
    XML_WS(L"japanese", pair.second.GetJapaneseTitle(), pugi::node_cdata);
    XML_WC(L"synonym", pair.second.GetSynonyms(), pugi::node_cdata);
    XML_WI(L"type", pair.second.GetType());
    XML_WI(L"status", pair.second.GetAiringStatus());
    XML_WI(L"episode_count", pair.second.GetEpisodeCount());
    XML_WI(L"episode_length", pair.second.GetEpisodeLength());
    XML_WD(L"date_start", pair.second.GetDateStart());
    XML_WD(L"date_end", pair.second.GetDateEnd());
    XML_WS(L"image", pair.second.GetImageUrl(), pugi::node_pcdata);
    XML_WI(L"age_rating", pair.second.GetAgeRating());
    XML_WS(L"genres", Join(pair.second.GetGenres(), L", "), pugi::node_pcdata);
    XML_WS(L"producers", Join(pair.second.GetProducers(), L", "), pugi::node_pcdata);
    XML_WF(L"score", pair.second.GetScore(), pugi::node_pcdata);
    XML_WI(L"popularity", pair.second.GetPopularity());
    XML_WS(L"synopsis", pair.second.GetSynopsis(), pugi::node_cdata);
    XML_WS(L"modified", ToWstr(pair.second.GetLastModified()), pugi::node_pcdata);
    #undef XML_WF
    #undef XML_WS
    #undef XML_WI
    #undef XML_WD
    #undef XML_WC
  }
}

////////////////////////////////////////////////////////////////////////////////

Item* Database::FindItem(int id, bool log_error) {
  if (IsValidId(id)) {
    auto it = items.find(id);
    if (it != items.end())
      return &it->second;
    if (log_error)
      LOGE(L"Could not find ID: {}", id);
  }

  return nullptr;
}

Item* Database::FindItem(const std::wstring& id, enum_t service,
                         bool log_error) {
  if (!id.empty()) {
    for (auto& pair : items)
      if (id == pair.second.GetId(service))
        return &pair.second;
    if (log_error)
      LOGE(L"Could not find ID: {}", id);
  }

  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////

void Database::ClearInvalidItems() {
  for (auto it = items.begin(); it != items.end(); ) {
    if (!anime::IsValidId(it->second.GetId()) ||
        it->first != it->second.GetId()) {
      LOGD(L"ID: {}", it->first);
      items.erase(it++);
    } else {
      ++it;
    }
  }
}

bool Database::DeleteItem(int id) {
  std::wstring title;

  auto anime_item = FindItem(id, false);
  if (anime_item)
    title = anime::GetPreferredTitle(*anime_item);

  if (items.erase(id) > 0) {
    LOGW(L"ID: {} | Title: {}", id, title);

    auto delete_history_items = [](int id, std::vector<HistoryItem>& items) {
      items.erase(std::remove_if(items.begin(), items.end(),
          [&id](const HistoryItem& item) {
            return item.anime_id == id;
          }), items.end());
    };

    delete_history_items(id, History.items);
    delete_history_items(id, History.queue.items);

    auto& items = SeasonDatabase.items;
    items.erase(std::remove(items.begin(), items.end(), id), items.end());

    if (CurrentEpisode.anime_id == id)
      CurrentEpisode.Set(anime::ID_UNKNOWN);

    ui::OnAnimeDelete(id, title);
    return true;
  }

  return false;
}

int Database::UpdateItem(const Item& new_item) {
  Item* item = nullptr;

  for (enum_t i = sync::kTaiga; i <= sync::kLastService; i++) {
    item = FindItem(new_item.GetId(i), i, false);
    if (item)
      break;
  }

  if (!item) {
    auto source = new_item.GetSource();

    if (source == sync::kTaiga) {
      LOGE(L"Invalid source for ID: {}", new_item.GetId(source));
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
    if (!new_item.GetJapaneseTitle().empty())
      item->SetJapaneseTitle(new_item.GetJapaneseTitle());
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
        !new_item.GetEnglishTitle(false).empty() ||
        !new_item.GetJapaneseTitle().empty())
      Meow.UpdateTitles(*item);
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

    item->SetMyId(new_item.GetMyId());
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
    item->SetMyNotes(new_item.GetMyNotes(false));
  }

  // Update local information
  {
    if (new_item.GetLastAiredEpisodeNumber())
      item->SetLastAiredEpisodeNumber(new_item.GetLastAiredEpisodeNumber());
    if (new_item.GetNextEpisodeTime())
      item->SetNextEpisodeTime(new_item.GetNextEpisodeTime());
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
  std::wstring path = taiga::GetPath(taiga::Path::UserLibrary);
  xml_parse_result parse_result = document.load_file(path.c_str());

  if (parse_result.status != pugi::status_ok) {
    if (parse_result.status == pugi::status_file_not_found) {
      if (CheckOldUserDirectory())
        return LoadList();
    } else {
      ui::DisplayErrorMessage(L"Could not read anime list.", path.c_str());
    }
    return false;
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
      anime_item.SetMyId(XmlReadStrValue(node, L"library_id"));
      anime_item.SetMyLastWatchedEpisode(XmlReadIntValue(node, L"progress"));
      anime_item.SetMyDateStart(XmlReadStrValue(node, L"date_start"));
      anime_item.SetMyDateEnd(XmlReadStrValue(node, L"date_end"));
      anime_item.SetMyScore(XmlReadIntValue(node, L"score"));
      anime_item.SetMyStatus(XmlReadIntValue(node, L"status"));
      anime_item.SetMyRewatchedTimes(XmlReadIntValue(node, L"rewatched_times"));
      anime_item.SetMyRewatching(XmlReadIntValue(node, L"rewatching"));
      anime_item.SetMyRewatchingEp(XmlReadIntValue(node, L"rewatching_ep"));
      anime_item.SetMyTags(XmlReadStrValue(node, L"tags"));
      anime_item.SetMyNotes(XmlReadStrValue(node, L"notes"));
      anime_item.SetMyLastUpdated(XmlReadStrValue(node, L"last_updated"));

      UpdateItem(anime_item);
    }

    HandleListCompatibility(meta_version);

  } else {
    LOGW(L"Reading list in compatibility mode");
    ReadListInCompatibilityMode(document);
  }

  return true;
}

bool Database::SaveList(bool include_database) {
  if (items.empty())
    return false;

  xml_document document;

  xml_node meta_node = document.append_child(L"meta");
  XmlWriteStrValue(meta_node, L"version", StrToWstr(Taiga.version.to_string()).c_str());

  if (include_database) {
    xml_node node_database = document.append_child(L"database");
    WriteDatabaseNode(node_database);
  }

  xml_node node_library = document.append_child(L"library");

  for (const auto& pair : items) {
    auto& item = pair.second;
    if (item.IsInList()) {
      xml_node node = node_library.append_child(L"anime");
      XmlWriteIntValue(node, L"id", item.GetId());
      XmlWriteStrValue(node, L"library_id", item.GetMyId().c_str());
      XmlWriteIntValue(node, L"progress", item.GetMyLastWatchedEpisode(false));
      XmlWriteStrValue(node, L"date_start", std::wstring(item.GetMyDateStart()).c_str());
      XmlWriteStrValue(node, L"date_end", std::wstring(item.GetMyDateEnd()).c_str());
      XmlWriteIntValue(node, L"score", item.GetMyScore(false));
      XmlWriteIntValue(node, L"status", item.GetMyStatus(false));
      XmlWriteIntValue(node, L"rewatched_times", item.GetMyRewatchedTimes());
      XmlWriteIntValue(node, L"rewatching", item.GetMyRewatching(false));
      XmlWriteIntValue(node, L"rewatching_ep", item.GetMyRewatchingEp());
      XmlWriteStrValue(node, L"tags", item.GetMyTags(false).c_str());
      XmlWriteStrValue(node, L"notes", item.GetMyNotes(false).c_str());
      XmlWriteStrValue(node, L"last_updated", item.GetMyLastUpdated().c_str());
    }
  }

  std::wstring path = taiga::GetPath(taiga::Path::UserLibrary);
  return XmlWriteDocumentToFile(document, path);
}

////////////////////////////////////////////////////////////////////////////////

int Database::GetItemCount(int status, bool check_history) {
  // Get current count
  int count = 0;
  for (const auto& it : items) {
    const auto& item = it.second;
    if (item.GetMyRewatching()) {
      if (status == kWatching)
        ++count;
    } else {
      if (item.GetMyStatus(false) == status)
        ++count;
    }
  }

  // Search queued items for status changes
  if (check_history) {
    for (const auto& history_item : History.queue.items) {
      if (history_item.status ||
          history_item.mode == taiga::kHttpServiceDeleteLibraryEntry) {
        if (status == *history_item.status) {
          count++;
        } else {
          auto anime_item = FindItem(history_item.anime_id);
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

  if (!anime_item || anime_item->IsInList())
    return;

  if (taiga::GetCurrentUsername().empty()) {
    ui::ChangeStatusText(
        L"Please set up your account before adding anime to your list.");
    return;
  }

  if (status == anime::kUnknownStatus)
    status = anime::IsAiredYet(*anime_item) ? anime::kWatching :
                                              anime::kPlanToWatch;

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

  SaveDatabase();
  SaveList();

  ui::OnLibraryEntryAdd(anime_id);

  if (CurrentEpisode.anime_id == anime::ID_NOTINLIST)
    CurrentEpisode.Set(anime::ID_UNKNOWN);
}

void Database::ClearUserData() {
  for (auto& pair : items)
    pair.second.RemoveFromUserList();
}

bool Database::DeleteListItem(int anime_id) {
  auto anime_item = FindItem(anime_id);

  if (!anime_item)
    return false;
  if (!anime_item->IsInList())
    return false;

  anime_item->RemoveFromUserList();

  ui::ChangeStatusText(L"Item deleted. (" + anime::GetPreferredTitle(*anime_item) + L")");
  ui::OnLibraryEntryDelete(anime_item->GetId());

  if (CurrentEpisode.anime_id == anime_id)
    CurrentEpisode.Set(anime::ID_NOTINLIST);

  return true;
}

void Database::UpdateItem(const HistoryItem& history_item) {
  auto anime_item = FindItem(history_item.anime_id);

  if (!anime_item)
    return;

  anime_item->AddtoUserList();

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
  // Edit notes
  if (history_item.notes) {
    anime_item->SetMyNotes(*history_item.notes);
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

  if (history_item.mode != taiga::kHttpServiceDeleteLibraryEntry)
    anime::SetMyLastUpdateToNow(*anime_item);

  ui::OnLibraryEntryChange(history_item.anime_id);
}

}  // namespace anime
