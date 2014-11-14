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

#include "base/log.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/anime_episode.h"
#include "library/anime_util.h"
#include "taiga/settings.h"
#include "track/monitor.h"
#include "track/recognition.h"
#include "track/search.h"

class FolderMonitor FolderMonitor;

void FolderMonitor::Enable(bool enabled) {
  Stop();
  Clear();

  if (enabled) {
    for (const auto& folder : Settings.root_folders)
      Add(folder);
    Start();
  }
}

static void ChangeAnimeFolder(anime::Item& anime_item,
                              const std::wstring& path) {
  anime_item.SetFolder(path);
  Settings.Save();

  LOG(LevelDebug, L"Anime folder changed: " + anime_item.GetTitle() + L"\n"
                  L"Path: " + anime_item.GetFolder());

  ScanAvailableEpisodesQuick(anime_item.GetId());
}

void FolderMonitor::HandleChangeNotification(
    const DirectoryChangeNotification& notification) const {
  switch (notification.type) {
    case DirectoryChangeNotification::kTypeDirectory:
      OnDirectory(notification);
      break;
    case DirectoryChangeNotification::kTypeFile:
      OnFile(notification);
      break;
    default:
      LOG(LevelDebug, L"Unknown change type\n"
                      L"Path: " + notification.path + L"\n"
                      L"Filename: " + notification.filename.first);
      break;
  }
}

static anime::Item* FindAnimeItem(const DirectoryChangeNotification& notification,
                                  anime::Episode& episode) {
  if (!Meow.Parse(notification.path + notification.filename.first, episode))
    return nullptr;

  static track::recognition::MatchOptions match_options;
  match_options.check_airing_date = false;
  match_options.check_anime_type = false;
  match_options.validate_episode_number = false;

  auto anime_id = Meow.Identify(episode, false, match_options);

  return AnimeDatabase.FindItem(anime_id);
}

void FolderMonitor::OnDirectory(const DirectoryChangeNotification& notification) const {
  anime::Item* anime_item = nullptr;

  bool new_path_available = notification.action != FILE_ACTION_REMOVED;
  bool old_path_available = notification.action >= FILE_ACTION_REMOVED;

  if (old_path_available) {
    std::wstring old_path = notification.path;
    old_path += notification.action == FILE_ACTION_REMOVED ?
        notification.filename.first : notification.filename.second;
    for (auto& item : AnimeDatabase.items) {
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
    if (anime_item)
      ChangeAnimeFolder(*anime_item, episode.folder + episode.file);
  }
}

void FolderMonitor::OnFile(const DirectoryChangeNotification& notification) const {
  anime::Episode episode;
  auto anime_item = FindAnimeItem(notification, episode);

  if (!anime_item)
    return;

  bool path_available = notification.action != FILE_ACTION_REMOVED;

  // Set anime folder
  if (path_available && anime_item->GetFolder().empty()) {
    ChangeAnimeFolder(*anime_item, episode.folder);
  }

  // Set episode availability
  int lower_bound = anime::GetEpisodeHigh(episode.number);
  int upper_bound = anime::GetEpisodeLow(episode.number);
  std::wstring path = notification.path + notification.filename.first;
  for (int number = lower_bound; number <= upper_bound; ++number) {
    if (anime_item->SetEpisodeAvailability(number, path_available, path)) {
      LOG(LevelDebug, anime_item->GetTitle() + L" #" + ToWstr(number) + L" is " +
          (path_available ? L"available." : L"unavailable."));
    }
  }
}