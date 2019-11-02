/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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

#include "media/anime_db.h"

#include "base/log.h"
#include "base/string.h"
#include "base/xml.h"
#include "media/anime_season_db.h"
#include "media/anime_util.h"
#include "media/library/history.h"
#include "media/library/queue.h"
#include "sync/service.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/version.h"
#include "track/recognition.h"
#include "ui/ui.h"

namespace anime {

bool Database::LoadDatabase() {
  const auto path = taiga::GetPath(taiga::Path::DatabaseAnime);
  constexpr auto options = pugi::parse_default & ~pugi::parse_eol;

  XmlDocument document;
  const auto parse_result = XmlLoadFileToDocument(document, path, options);

  if (!parse_result)
    return false;

  const auto meta_version = XmlReadMetaVersion(document);

  auto database_node = document.child(L"database");
  ReadDatabaseNode(database_node);

  HandleCompatibility(meta_version);

  return true;
}

void Database::ReadDatabaseNode(XmlNode& database_node) {
  for (auto node : database_node.children(L"anime")) {
    std::map<enum_t, std::wstring> id_map;

    for (auto id_node : node.children(L"id")) {
      const std::wstring slug = id_node.attribute(L"name").as_string();
      const enum_t service_id = sync::GetServiceIdBySlug(slug);
      id_map[service_id] = id_node.child_value();
    }

    const std::wstring source_name = XmlReadStr(node, L"source");
    enum_t source = sync::GetServiceIdBySlug(source_name);

    if (source == sync::kTaiga) {
      const auto current_service_id = sync::GetCurrentServiceId();
      if (id_map.find(current_service_id) != id_map.end()) {
        source = current_service_id;
        LOGW(L"Fixed source for ID: {}", id_map[source]);
      } else {
        LOGE(L"Invalid source for ID: {}", id_map[sync::kTaiga]);
        continue;
      }
    }

    const int id = ToInt(id_map[sync::kTaiga]);
    Item& item = items[id];  // Creates the item if it doesn't exist

    for (const auto& [service, id] : id_map) {
      item.SetId(id, service);
    }

    item.SetSource(source);
    item.SetTitle(XmlReadStr(node, L"title"));
    item.SetType(XmlReadInt(node, L"type"));
    item.SetAiringStatus(XmlReadInt(node, L"status"));
    item.SetAgeRating(XmlReadInt(node, L"age_rating"));
    item.SetGenres(XmlReadStr(node, L"genres"));
    item.SetProducers(XmlReadStr(node, L"producers"));
    item.SetSynopsis(XmlReadStr(node, L"synopsis"));
    item.SetLastModified(ToTime(XmlReadStr(node, L"modified")));
    item.SetEnglishTitle(XmlReadStr(node, L"english"));
    item.SetJapaneseTitle(XmlReadStr(node, L"japanese"));
    for (auto child_node : node.children(L"synonym")) {
      item.InsertSynonym(child_node.child_value());
    }
    item.SetPopularity(XmlReadInt(node, L"popularity"));
    item.SetScore(ToDouble(XmlReadStr(node, L"score")));
    item.SetDateEnd(Date(XmlReadStr(node, L"date_end")));
    item.SetDateStart(Date(XmlReadStr(node, L"date_start")));
    item.SetEpisodeLength(XmlReadInt(node, L"episode_length"));
    item.SetEpisodeCount(XmlReadInt(node, L"episode_count"));
    item.SetSlug(XmlReadStr(node, L"slug"));
    item.SetImageUrl(XmlReadStr(node, L"image"));
  }
}

bool Database::SaveDatabase() const {
  XmlDocument document;

  XmlWriteMetaVersion(document, StrToWstr(taiga::version().to_string()));
  WriteDatabaseNode(XmlChild(document, L"database"));

  const auto path = taiga::GetPath(taiga::Path::DatabaseAnime);
  return XmlSaveDocumentToFile(document, path);
}

void Database::WriteDatabaseNode(XmlNode& database_node) const {
  for (const auto& [id, item] : items) {
    auto anime_node = database_node.append_child(L"anime");

    for (int i = 0; i <= sync::kLastService; i++) {
      std::wstring id = item.GetId(i);
      if (!id.empty()) {
        auto child = anime_node.append_child(L"id");
        std::wstring slug = sync::GetServiceSlugById(static_cast<sync::ServiceId>(i));
        child.append_attribute(L"name") = slug.c_str();
        child.append_child(pugi::node_pcdata).set_value(id.c_str());
      }
    }

    const std::wstring source = sync::GetServiceSlugById(
        static_cast<sync::ServiceId>(item.GetSource()));

    #define XML_WC(n, v, t) \
      if (!v.empty()) XmlWriteChildNodes(anime_node, v, n, t)
    #define XML_WD(n, v) \
      if (v) XmlWriteStr(anime_node, n, v.to_string())
    #define XML_WI(n, v) \
      if (v > 0) XmlWriteInt(anime_node, n, v)
    #define XML_WS(n, v, t) \
      if (!v.empty()) XmlWriteStr(anime_node, n, v, t)
    #define XML_WF(n, v, t) \
      if (v > 0.0) XmlWriteStr(anime_node, n, ToWstr(v), t)
    XML_WS(L"source", source, pugi::node_pcdata);
    XML_WS(L"slug", item.GetSlug(), pugi::node_pcdata);
    XML_WS(L"title", item.GetTitle(), pugi::node_cdata);
    XML_WS(L"english", item.GetEnglishTitle(), pugi::node_cdata);
    XML_WS(L"japanese", item.GetJapaneseTitle(), pugi::node_cdata);
    XML_WC(L"synonym", item.GetSynonyms(), pugi::node_cdata);
    XML_WI(L"type", item.GetType());
    XML_WI(L"status", item.GetAiringStatus());
    XML_WI(L"episode_count", item.GetEpisodeCount());
    XML_WI(L"episode_length", item.GetEpisodeLength());
    XML_WD(L"date_start", item.GetDateStart());
    XML_WD(L"date_end", item.GetDateEnd());
    XML_WS(L"image", item.GetImageUrl(), pugi::node_pcdata);
    XML_WI(L"age_rating", item.GetAgeRating());
    XML_WS(L"genres", Join(item.GetGenres(), L", "), pugi::node_pcdata);
    XML_WS(L"producers", Join(item.GetProducers(), L", "), pugi::node_pcdata);
    XML_WF(L"score", item.GetScore(), pugi::node_pcdata);
    XML_WI(L"popularity", item.GetPopularity());
    XML_WS(L"synopsis", item.GetSynopsis(), pugi::node_cdata);
    XML_WS(L"modified", ToWstr(item.GetLastModified()), pugi::node_pcdata);
    #undef XML_WF
    #undef XML_WS
    #undef XML_WI
    #undef XML_WD
    #undef XML_WC
  }
}

////////////////////////////////////////////////////////////////////////////////

Item* Database::Find(int id, bool log_error) {
  if (IsValidId(id)) {
    auto it = items.find(id);
    if (it != items.end())
      return &it->second;
    if (log_error)
      LOGE(L"Could not find ID: {}", id);
  }

  return nullptr;
}

Item* Database::Find(const std::wstring& id, enum_t service, bool log_error) {
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

  auto anime_item = Find(id, false);
  if (anime_item)
    title = anime::GetPreferredTitle(*anime_item);

  if (items.erase(id) > 0) {
    LOGW(L"ID: {} | Title: {}", id, title);

    library::history.items.erase(
        std::remove_if(library::history.items.begin(),
                       library::history.items.end(),
                       [&id](const library::HistoryItem& item) {
                         return item.anime_id == id;
                       }),
        library::history.items.end());

    library::queue.items.erase(
        std::remove_if(library::queue.items.begin(),
                       library::queue.items.end(),
                       [&id](const library::QueueItem& item) {
                         return item.anime_id == id;
                       }),
        library::queue.items.end());

    auto& items = anime::season_db.items;
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
    item = Find(new_item.GetId(i), i, false);
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

}  // namespace anime
