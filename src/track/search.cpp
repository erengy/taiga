/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "track/recognition.h"
#include "track/search.h"
#include "ui/ui.h"

TaigaFileSearchHelper file_search_helper;

TaigaFileSearchHelper::TaigaFileSearchHelper()
    : anime_id_(anime::ID_UNKNOWN),
      episode_number_(0) {
}

bool TaigaFileSearchHelper::OnDirectory(const std::wstring& root,
                                        const std::wstring& name,
                                        const WIN32_FIND_DATA& data) {
  static track::recognition::ParseOptions parse_options;
  parse_options.parse_path = false;
  parse_options.streaming_media = false;

  if (!Meow.Parse(name, parse_options, episode_)) {
    LOGD(L"Could not parse directory: " + name);
    return false;
  }

  static track::recognition::MatchOptions match_options;
  match_options.allow_sequels = false;
  match_options.check_airing_date = false;
  match_options.check_anime_type = false;
  match_options.check_episode_number = false;

  Meow.Identify(episode_, false, match_options);

  anime::Item* anime_item = AnimeDatabase.FindItem(episode_.anime_id);

  if (anime_item && Meow.IsValidAnimeType(episode_)) {
    if (anime_item->GetFolder().empty())
      anime_item->SetFolder(AddTrailingSlash(root) + name);

    if (anime::IsValidId(anime_id_) && anime_id_ == anime_item->GetId()) {
      path_found_ = AddTrailingSlash(root) + name;
      if (skip_files_)
        return true;
    }
  }

  return false;
}

bool TaigaFileSearchHelper::OnFile(const std::wstring& root,
                                   const std::wstring& name,
                                   const WIN32_FIND_DATA& data) {
  auto path = AddTrailingSlash(root) + name;

  static track::recognition::ParseOptions parse_options;
  parse_options.parse_path = true;
  parse_options.streaming_media = false;

  if (!Meow.Parse(path, parse_options, episode_)) {
    LOGD(L"Could not parse filename: " + name);
    return false;
  }

  static track::recognition::MatchOptions match_options;
  match_options.allow_sequels = true;
  match_options.check_airing_date = true;
  match_options.check_anime_type = true;
  match_options.check_episode_number = true;

  Meow.Identify(episode_, false, match_options);

  anime::Item* anime_item = AnimeDatabase.FindItem(episode_.anime_id);

  if (anime_item && Meow.IsValidAnimeType(episode_) &&
                    Meow.IsValidFileExtension(episode_)) {
    int upper_bound = anime::GetEpisodeHigh(episode_);
    int lower_bound = anime::GetEpisodeLow(episode_);

    if (!anime::IsValidEpisodeNumber(upper_bound, anime_item->GetEpisodeCount()) ||
        !anime::IsValidEpisodeNumber(lower_bound, anime_item->GetEpisodeCount())) {
      std::wstring episode_number = anime::GetEpisodeRange(episode_);
      LOGD(L"Invalid episode number: " + episode_number + L"\n"
           L"File: " + path);
      return false;
    }

    for (int i = lower_bound; i <= upper_bound; ++i)
      anime_item->SetEpisodeAvailability(i, true, path);

    if (anime::IsValidId(anime_id_) && anime_id_ == anime_item->GetId()) {
      // Check if we've found the episode we were looking for
      if (episode_number_ > 0 &&
          episode_number_ >= lower_bound && episode_number_ <= upper_bound) {
        path_found_ = path;
        return true;
      }
      // Check if all episodes are available
      if (episode_number_ == 0 &&
          IsAllEpisodesAvailable(*anime_item)) {
        return true;
      }
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

const std::wstring& TaigaFileSearchHelper::path_found() const {
  return path_found_;
}

void TaigaFileSearchHelper::set_anime_id(int anime_id) {
  anime_id_ = anime_id;
}

void TaigaFileSearchHelper::set_episode_number(int episode_number) {
  episode_number_ = episode_number;
}

void TaigaFileSearchHelper::set_path_found(const std::wstring& path_found) {
  path_found_ = path_found;
}

////////////////////////////////////////////////////////////////////////////////

void ScanAvailableEpisodes(bool silent) {
  for (auto& pair : AnimeDatabase.items) {
    anime::ValidateFolder(pair.second);
  }

  ScanAvailableEpisodes(silent, anime::ID_UNKNOWN, 0);
}

void ScanAvailableEpisodes(bool silent, int anime_id, int episode_number) {
  // Check if any library folder is available
  if (!silent && Settings.library_folders.empty()) {
    ui::OnSettingsLibraryFoldersEmpty();
    return;
  }

  if (!silent) {
    ui::taskbar_list.SetProgressState(TBPF_INDETERMINATE);
    ui::SetSharedCursor(IDC_WAIT);
    ui::ChangeStatusText(L"Scanning available episodes...");
  }

  file_search_helper.set_anime_id(anime_id);
  file_search_helper.set_episode_number(episode_number);
  // Casting file size threshold to int shouldn't be a problem, as the value
  // is never supposed to exceed INT_MAX (i.e. ~2GB).
  file_search_helper.set_minimum_file_size(
      Settings.GetInt(taiga::kLibrary_FileSizeThreshold));
  file_search_helper.set_path_found(L"");

  auto anime_item = AnimeDatabase.FindItem(anime_id);
  bool found = false;

  if (anime_item) {
    // Check if the anime folder still exists
    anime::ValidateFolder(*anime_item);

    // Search the anime folder for available episodes
    if (!anime_item->GetFolder().empty()) {
      file_search_helper.set_skip_directories(true);
      file_search_helper.set_skip_files(false);
      file_search_helper.set_skip_subdirectories(false);
      found = file_search_helper.Search(anime_item->GetFolder());
    }

    // Search the cached episode path
    if (!found && !anime_item->GetNextEpisodePath().empty()) {
      std::wstring next_episode_path = GetPathOnly(anime_item->GetNextEpisodePath());
      if (!IsEqual(next_episode_path, anime_item->GetFolder())) {
        file_search_helper.set_skip_directories(true);
        file_search_helper.set_skip_files(false);
        file_search_helper.set_skip_subdirectories(true);
        found = file_search_helper.Search(next_episode_path);
      }
    }
  }

  if (!found) {
    // Search library folders for available episodes
    for (const auto& folder : Settings.library_folders) {
      if (!FolderExists(folder))
        continue;  // Might be a disconnected external drive
      bool skip_directories = false;
      if (anime_item && !anime_item->GetFolder().empty())
        skip_directories = true;
      file_search_helper.set_skip_directories(skip_directories);
      file_search_helper.set_skip_files(false);
      file_search_helper.set_skip_subdirectories(false);
      if (file_search_helper.Search(folder)) {
        found = true;
        break;
      }
    }
  }

  if (!silent) {
    ui::taskbar_list.SetProgressState(TBPF_NOPROGRESS);
    ui::SetSharedCursor(IDC_ARROW);
    ui::ClearStatusText();
  }

  ui::OnScanAvailableEpisodesFinished();
}

void ScanAvailableEpisodesQuick() {
  ScanAvailableEpisodesQuick(anime::ID_UNKNOWN);
}

void ScanAvailableEpisodesQuick(int anime_id) {
  foreach_r_(it, AnimeDatabase.items) {
    anime::Item& anime_item = it->second;

    if (anime_id != anime::ID_UNKNOWN && anime_item.GetId() != anime_id)
      continue;
    if (anime_item.GetFolder().empty())
      continue;
    if (!FolderExists(anime_item.GetFolder()))
      continue;

    file_search_helper.set_anime_id(anime_item.GetId());
    file_search_helper.set_episode_number(0);
    file_search_helper.set_minimum_file_size(
        Settings.GetInt(taiga::kLibrary_FileSizeThreshold));
    file_search_helper.set_skip_directories(true);
    file_search_helper.set_skip_files(false);
    file_search_helper.set_skip_subdirectories(false);

    file_search_helper.Search(anime_item.GetFolder());
  }

  ui::OnScanAvailableEpisodesFinished();
}