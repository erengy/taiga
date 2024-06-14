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

#include "track/monitor.h"

#include "base/log.h"
#include "base/string.h"
#include "media/anime_db.h"
#include "taiga/settings.h"
#include "track/episode.h"
#include "track/episode_util.h"
#include "track/recognition.h"
#include "track/scanner.h"

namespace track {

static void ChangeAnimeFolder(anime::Item& anime_item,
                              const std::wstring& path) {
  anime_item.SetFolder(path);
  taiga::settings.Save();

  LOGD(L"Anime folder changed: {}\nPath: {}",
       anime_item.GetTitle(), anime_item.GetFolder());

  if (path.empty()) {
    for (int i = 1; i <= anime_item.GetAvailableEpisodeCount(); ++i) {
      anime_item.SetEpisodeAvailability(i, false, path);
    }
  }

  ScanAvailableEpisodesQuick(anime_item.GetId());
}

static anime::Item* FindAnimeItem(
    const DirectoryChangeNotification& notification, anime::Episode& episode) {
  std::wstring path;
  static track::recognition::ParseOptions parse_options;
  switch (notification.type) {
    case DirectoryChangeNotification::Type::Directory:
      path = GetFileName(notification.filename.first);
      parse_options.parse_path = false;
      parse_options.streaming_media = false;
      break;
    default:
    case DirectoryChangeNotification::Type::File:
      path = notification.path + notification.filename.first;
      parse_options.parse_path = true;
      parse_options.streaming_media = false;
      break;
  }

  if (!Meow.Parse(path, parse_options, episode))
    return nullptr;

  static track::recognition::MatchOptions match_options;
  match_options.streaming_media = false;
  switch (notification.type) {
    case DirectoryChangeNotification::Type::Directory:
      match_options.allow_sequels = false;
      match_options.check_airing_date = false;
      match_options.check_anime_type = false;
      match_options.check_episode_number = false;
      break;
    default:
    case DirectoryChangeNotification::Type::File:
      match_options.allow_sequels = true;
      match_options.check_airing_date = true;
      match_options.check_anime_type = true;
      match_options.check_episode_number = true;
      break;
  }

  const auto anime_id = Meow.Identify(episode, false, match_options);

  return anime::db.Find(anime_id);
}

////////////////////////////////////////////////////////////////////////////////

void Monitor::Enable(bool enabled) {
  Stop();
  Clear();

  if (enabled) {
    for (const auto& folder : taiga::settings.GetLibraryFolders()) {
      Add(folder);
    }
    Start();
  }
}

void Monitor::HandleChangeNotification(
    const DirectoryChangeNotification& notification) const {
  switch (notification.type) {
    case DirectoryChangeNotification::Type::Directory:
      OnDirectory(notification);
      break;
    case DirectoryChangeNotification::Type::File:
      OnFile(notification);
      break;
    default:
      LOGD(L"Unknown change type\nPath: {}\nFilename: {}",
           notification.path, notification.filename.first);
      break;
  }
}

void Monitor::OnDirectory(
    const DirectoryChangeNotification& notification) const {
  anime::Item* anime_item = nullptr;

  const bool new_path_available = notification.action != FILE_ACTION_REMOVED;
  const bool old_path_available = notification.action >= FILE_ACTION_REMOVED;

  if (old_path_available) {
    std::wstring old_path = notification.path;
    old_path += notification.action == FILE_ACTION_REMOVED ?
        notification.filename.first : notification.filename.second;
    for (auto& item : anime::db.items) {
      if (IsEqual(item.second.GetFolder(), old_path)) {
        anime_item = &item.second;
        break;
      }
    }
    if (anime_item) {
      std::wstring new_path = notification.path + notification.filename.first;
      ChangeAnimeFolder(*anime_item, new_path_available ? new_path : L"");
      return;
    }
  }

  if (new_path_available) {
    anime::Episode episode;
    anime_item = FindAnimeItem(notification, episode);
    if (anime_item && Meow.IsValidAnimeType(episode)) {
      std::wstring new_path = notification.path + notification.filename.first;
      ChangeAnimeFolder(*anime_item, new_path);
    }
  }
}

void Monitor::OnFile(const DirectoryChangeNotification& notification) const {
  anime::Episode episode;
  const auto anime_item = FindAnimeItem(notification, episode);

  if (!anime_item)
    return;
  if (!Meow.IsValidAnimeType(episode) || !Meow.IsValidFileExtension(episode))
    return;

  const bool path_available = notification.action != FILE_ACTION_REMOVED;

  // Set anime folder
  if (path_available && anime_item->GetFolder().empty()) {
    ChangeAnimeFolder(*anime_item, episode.folder);
  }

  // Set episode availability
  const int lower_bound = anime::GetEpisodeLow(episode);
  const int upper_bound = anime::GetEpisodeHigh(episode);
  const std::wstring path = notification.path + notification.filename.first;
  for (int number = lower_bound; number <= upper_bound; ++number) {
    if (anime_item->SetEpisodeAvailability(number, path_available, path)) {
      LOGD(L"{} #{} is {}.", anime_item->GetTitle(), number,
           path_available ? L"available" : L"unavailable");
    }
  }
}

}  // namespace track
