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

#include <filesystem>

#include "taiga/settings.h"

#include "base/log.h"
#include "base/string.h"
#include "base/xml.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "media/library/history.h"
#include "sync/service.h"
#include "sync/sync.h"
#include "taiga/path.h"
#include "taiga/stats.h"
#include "taiga/timer.h"
#include "taiga/version.h"
#include "track/feed_aggregator.h"
#include "track/feed_filter_manager.h"
#include "ui/menu.h"
#include "ui/ui.h"

namespace taiga {

Settings::~Settings() {
  Save();
}

bool Settings::Load() {
  std::lock_guard lock{mutex_};

  if (modified_) {
    LOGE(L"Cannot load settings when current values are modified.");
    return false;
  }

  const auto path = taiga::GetPath(taiga::Path::Settings);
  const auto old_path = path + L".old";
  const auto new_path = path + L".new";

  std::error_code error;
  if (std::filesystem::exists(new_path, error)) {
    if (DeserializeFromXml(new_path)) {
      LOGW(L"Found new settings.");
      std::filesystem::rename(path, old_path, error);
      std::filesystem::rename(new_path, path, error);
      return true;
    }
  }

  return DeserializeFromXml(path);
}

bool Settings::Save() {
  std::lock_guard lock{mutex_};

  if (!modified_) {
    return false;
  }

  const auto path = taiga::GetPath(taiga::Path::Settings);
  const auto new_path = path + L".new";

  if (!SerializeToXml(new_path)) {
    LOGE(L"Could not save application settings.\nPath: {}", new_path);
    return false;
  }

  std::error_code error;
  std::filesystem::rename(new_path, path, error);
  if (error) {
    LOGE(StrToWstr(error.message()));
    return false;
  }

  modified_ = false;

  return true;
}

bool Settings::DeserializeFromXml(const std::wstring& path) {
  XmlDocument document;
  const auto parse_result = XmlLoadFileToDocument(document, path);

  if (!parse_result) {
    LOGE(L"Could not read application settings.\nPath: {}\nReason: {}",
         path, StrToWstr(parse_result.description()));
  }

  const auto attr_from_path = [](XmlNode node, const std::string& path) {
    std::vector<std::wstring> node_names;
    Split(StrToWstr(path), L"/", node_names);
    const auto attr_name = node_names.back();
    node_names.pop_back();
    for (const auto& node_name : node_names) {
      node = node.child(node_name.c_str());
    }
    return node.attribute(attr_name.c_str());
  };

  const auto settings = document.child(L"settings");

  InitKeyMap();
  for (const auto& [key, app_setting] : key_map_) {
    const auto attr = attr_from_path(settings, app_setting.key);
    switch (base::GetSettingValueType(app_setting.default_value)) {
      case base::SettingValueType::Bool:
        settings_.set_value(app_setting.key, attr.as_bool(
            std::get<bool>(app_setting.default_value)));
        break;
      case base::SettingValueType::Int:
        settings_.set_value(app_setting.key, attr.as_int(
            std::get<int>(app_setting.default_value)));
        break;
      case base::SettingValueType::Wstring:
        settings_.set_value(app_setting.key, std::wstring{attr.as_string(
            std::get<std::wstring>(app_setting.default_value).c_str())});
        break;
    }
  }

  // Folders
  library_folders_.clear();
  const auto node_folders = settings.child(L"anime").child(L"folders");
  for (const auto folder : node_folders.children(L"root")) {
    library_folders_.push_back(folder.attribute(L"folder").value());
  }

  // Anime items
  const auto node_items = settings.child(L"anime").child(L"items");
  for (const auto item : node_items.children(L"item")) {
    const int anime_id = item.attribute(L"id").as_int();
    if (anime::IsValidId(anime_id)) {
      auto& anime_item = anime_settings_[anime_id];
      anime_item.folder = item.attribute(L"folder").value();
      Split(item.attribute(L"titles").value(), L"; ", anime_item.synonyms);
      RemoveEmptyStrings(anime_item.synonyms);
      anime_item.use_alternative = item.attribute(L"use_alternative").as_bool();
    }
  }

  // Media players
  const auto node_players = settings.child(L"recognition").child(L"mediaplayers");
  for (const auto player : node_players.children(L"player")) {
    media_players_enabled_[player.attribute(L"name").value()] =
        player.attribute(L"enabled").as_bool();
  }

  // Anime list columns
  const auto node_list_columns = settings.child(L"program").child(L"list").child(L"columns");
  for (const auto column : node_list_columns.children(L"column")) {
    const std::wstring key = column.attribute(L"name").value();
    auto& data = anime_list_columns_[key];
    data.order = column.attribute(L"order").as_int();
    data.visible = column.attribute(L"visible").as_bool();
    data.width = column.attribute(L"width").as_int();
  }

  // Torrent filters
  const auto node_filter = settings.child(L"rss").child(L"torrent").child(L"filter");
  track::feed_filter_manager.Import(node_filter);
  track::feed_filter_manager.AddPresets();

  return parse_result;
}

bool Settings::SerializeToXml(const std::wstring& path) const {
  XmlDocument document;

  auto settings = document.append_child(L"settings");

  const auto attr_from_path = [](XmlNode node, const std::string& path) {
    std::vector<std::wstring> node_names;
    Split(StrToWstr(path), L"/", node_names);
    const auto attr_name = node_names.back();
    node_names.pop_back();
    for (const auto& node_name : node_names) {
      node = XmlChild(node, node_name.c_str());
    }
    return XmlAttr(node, attr_name.c_str());
  };

  InitKeyMap();
  for (const auto& [key, app_setting] : key_map_) {
    auto attr = attr_from_path(settings, app_setting.key);
    const auto value = settings_.value(app_setting.key);

    switch (base::GetSettingValueType(value)) {
      case base::SettingValueType::Bool:
        attr.set_value(std::get<bool>(value));
        break;
      case base::SettingValueType::Int:
        attr.set_value(std::get<int>(value));
        break;
      case base::SettingValueType::Wstring:
        attr.set_value(std::get<std::wstring>(value).c_str());
        break;
    }
  }

  // Meta
  auto meta = XmlChild(settings, L"meta");
  XmlAttr(meta, L"version").set_value(StrToWstr(taiga::version().to_string()).c_str());

  // Library folders
  auto folders = settings.child(L"anime").child(L"folders");
  for (const auto& folder : library_folders_) {
    auto root = folders.append_child(L"root");
    root.append_attribute(L"folder") = folder.c_str();
  }

  // Anime items
  auto items = settings.child(L"anime").append_child(L"items");
  for (const auto& [anime_id, anime_item] : anime_settings_) {
    if (anime_item.folder.empty() &&
        anime_item.synonyms.empty() &&
        !anime_item.use_alternative) {
      continue;
    }
    auto item = items.append_child(L"item");
    item.append_attribute(L"id") = anime_id;
    if (!anime_item.folder.empty())
      item.append_attribute(L"folder") = anime_item.folder.c_str();
    if (!anime_item.synonyms.empty())
      item.append_attribute(L"titles") = Join(anime_item.synonyms, L"; ").c_str();
    if (anime_item.use_alternative)
      item.append_attribute(L"use_alternative") = anime_item.use_alternative;
  }

  // Media players
  auto mediaplayers = settings.child(L"recognition").child(L"mediaplayers");
  for (const auto& [name, enabled] : media_players_enabled_) {
    auto player = mediaplayers.append_child(L"player");
    player.append_attribute(L"name") = name.c_str();
    player.append_attribute(L"enabled") = enabled;
  }

  // Anime list columns
  auto list_columns = settings.child(L"program").child(L"list").append_child(L"columns");
  for (const auto& [key, column] : anime_list_columns_) {
    auto node = list_columns.append_child(L"column");
    node.append_attribute(L"name") = key.c_str();
    node.append_attribute(L"order") = column.order;
    node.append_attribute(L"visible") = column.visible;
    node.append_attribute(L"width") = column.width;
  }

  // Torrent filters
  auto torrent_filter = settings.child(L"rss").child(L"torrent").child(L"filter");
  track::feed_filter_manager.Export(torrent_filter);

  return XmlSaveDocumentToFile(document, path);
}

////////////////////////////////////////////////////////////////////////////////

void Settings::ApplyChanges() {
  if (changed_account_or_service_) {
    anime::db.LoadList();
    library::history.Load();
    CurrentEpisode.Set(anime::ID_UNKNOWN);
    taiga::stats.CalculateAll();
    sync::InvalidateUserAuthentication();
    ui::OnSettingsUserChange();
    ui::OnSettingsServiceChange();
    changed_account_or_service_ = false;
  } else {
    ui::OnSettingsChange();
  }

  ui::Menus.UpdateFolders();

  timers.UpdateIntervalsFromSettings();
}

void Settings::SetModified() {
  std::lock_guard lock{mutex_};
  modified_ = true;
}

////////////////////////////////////////////////////////////////////////////////

std::wstring GetCurrentUserDisplayName() {
  switch (sync::GetCurrentServiceId()) {
    case sync::ServiceId::Kitsu: {
      const auto display_name = settings.GetSyncServiceKitsuDisplayName();
      if (!display_name.empty())
        return display_name;
      break;
    }
  }

  return GetCurrentUsername();
}

std::wstring GetCurrentUsername() {
  switch (sync::GetCurrentServiceId()) {
    case sync::ServiceId::MyAnimeList:
      return settings.GetSyncServiceMalUsername();
    case sync::ServiceId::Kitsu:
      return settings.GetSyncServiceKitsuUsername();
    case sync::ServiceId::AniList:
      return settings.GetSyncServiceAniListUsername();
    default:
      return {};
  }
}

////////////////////////////////////////////////////////////////////////////////

int Settings::AnimeListColumn::compare(
  const Settings::AnimeListColumn& rhs) const {
  if (order != rhs.order) return nstd::compare(order, rhs.order);
  if (visible != rhs.visible) return nstd::compare(visible, rhs.visible);
  if (width != rhs.width) return nstd::compare(width, rhs.width);
  return nstd::cmp::equal;
}

}  // namespace taiga
