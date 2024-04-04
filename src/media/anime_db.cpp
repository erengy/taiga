/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <nstd/algorithm.hpp>

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
    std::map<sync::ServiceId, std::wstring> id_map;

    for (auto id_node : node.children(L"id")) {
      const std::wstring slug = id_node.attribute(L"name").as_string();
      const auto service_id = sync::GetServiceIdBySlug(slug);
      if (service_id != sync::ServiceId::Unknown)
        id_map[service_id] = id_node.child_value();
    }

    auto source = sync::GetServiceIdBySlug(XmlReadStr(node, L"source"));
    if (source == sync::ServiceId::Unknown) {
      const auto current_service_id = sync::GetCurrentServiceId();
      if (nstd::contains(id_map, current_service_id)) {
        source = current_service_id;
        LOGW(L"Fixed source to {} ({}).", sync::GetCurrentServiceName(),
             id_map[source]);
      } else {
        LOGE(L"Discarding data from unknown source.");
        continue;
      }
    }

    const int id = ToInt(id_map[sync::GetCurrentServiceId()]);
    Item& item = items[id];  // Creates the item if it doesn't exist

    for (const auto& [service, id] : id_map) {
      item.SetId(id, service);
    }

    item.SetSource(source);
    item.SetTitle(XmlReadStr(node, L"title"));
    item.SetType(static_cast<SeriesType>(XmlReadInt(node, L"type")));
    item.SetAiringStatus(static_cast<SeriesStatus>(XmlReadInt(node, L"status")));
    item.SetAgeRating(static_cast<AgeRating>(XmlReadInt(node, L"age_rating")));
    item.SetGenres(XmlReadStr(node, L"genres"));
    item.SetTags(XmlReadStr(node, L"tags"));
    item.SetProducers(XmlReadStr(node, L"producers"));
    item.SetStudios(XmlReadStr(node, L"studios"));
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
    item.SetTrailerId(XmlReadStr(node, L"trailer_id"));
    item.SetLastAiredEpisodeNumber(XmlReadInt(node, L"last_aired_episode"));
    item.SetNextEpisodeTime(ToTime(XmlReadStr(node, L"next_episode_time")));
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

    for (const auto service_id : sync::kServiceIds) {
      const auto id = item.GetId(service_id);
      if (!id.empty()) {
        auto child = anime_node.append_child(L"id");
        const auto slug = sync::GetServiceSlugById(service_id);
        child.append_attribute(L"name") = slug.c_str();
        child.append_child(pugi::node_pcdata).set_value(id.c_str());
      }
    }

    const std::wstring source = sync::GetServiceSlugById(
        static_cast<sync::ServiceId>(item.GetSource()));

    #define XML_WC(n, v, t) \
      if (!v.empty()) XmlWriteChildNodes(anime_node, v, n, t)
    #define XML_WD(n, v) \
      if (!v.empty()) XmlWriteStr(anime_node, n, v.to_string())
    #define XML_WI(n, v) \
      if (v > 0) XmlWriteInt(anime_node, n, v)
    #define XML_WS(n, v, t) \
      if (!v.empty()) XmlWriteStr(anime_node, n, v, t)
    #define XML_WF(n, v, t) \
      if (v > 0.0) XmlWriteStr(anime_node, n, ToWstr(v), t)
    #define XML_WT(n, v, t) \
      if (v > 0) XmlWriteStr(anime_node, n, ToWstr(v), t)
    XML_WS(L"source", source, pugi::node_pcdata);
    XML_WS(L"slug", item.GetSlug(), pugi::node_pcdata);
    XML_WS(L"title", item.GetTitle(), pugi::node_cdata);
    XML_WS(L"english", item.GetEnglishTitle(), pugi::node_cdata);
    XML_WS(L"japanese", item.GetJapaneseTitle(), pugi::node_cdata);
    XML_WC(L"synonym", item.GetSynonyms(), pugi::node_cdata);
    XML_WI(L"type", static_cast<int>(item.GetType()));
    XML_WI(L"status", static_cast<int>(item.GetAiringStatus(false)));
    XML_WI(L"episode_count", item.GetEpisodeCount());
    XML_WI(L"episode_length", item.GetEpisodeLength());
    XML_WD(L"date_start", item.GetDateStart());
    XML_WD(L"date_end", item.GetDateEnd());
    XML_WS(L"image", item.GetImageUrl(), pugi::node_pcdata);
    XML_WS(L"trailer_id", item.GetTrailerId(), pugi::node_pcdata);
    XML_WI(L"age_rating", static_cast<int>(item.GetAgeRating()));
    XML_WS(L"genres", Join(item.GetGenres(), L", "), pugi::node_pcdata);
    XML_WS(L"tags", Join(item.GetTags(), L", "), pugi::node_pcdata);
    XML_WS(L"producers", Join(item.GetProducers(), L", "), pugi::node_pcdata);
    XML_WS(L"studios", Join(item.GetStudios(), L", "), pugi::node_pcdata);
    XML_WF(L"score", item.GetScore(), pugi::node_pcdata);
    XML_WI(L"popularity", item.GetPopularity());
    XML_WS(L"synopsis", item.GetSynopsis(), pugi::node_cdata);
    XML_WI(L"last_aired_episode", item.GetLastAiredEpisodeNumber());
    XML_WT(L"next_episode_time", item.GetNextEpisodeTime(), pugi::node_pcdata);
    XML_WT(L"modified", item.GetLastModified(), pugi::node_pcdata);
    #undef XML_WT
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

Item* Database::Find(const std::wstring& id, sync::ServiceId service,
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

}  // namespace anime
