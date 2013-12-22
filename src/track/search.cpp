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

#include "base/common.h"
#include "base/foreach.h"
#include "base/logger.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "track/recognition.h"
#include "ui/ui.h"
#include "win/win_taskbar.h"

// Extends the length limit from 260 to 32767 characters
std::wstring GetExtendedLengthPath(const std::wstring& path) {
  if (!StartsWith(path, L"\\\\?\\"))
    return L"\\\\?\\" + path;
  return path;
}

bool IsDirectory(const WIN32_FIND_DATA& win32_find_data) {
  return (win32_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool IsValidDirectory(const WIN32_FIND_DATA& win32_find_data) {
  return wcscmp(win32_find_data.cFileName, L".") != 0 &&
         wcscmp(win32_find_data.cFileName, L"..") != 0;
}

std::wstring SearchFileFolder(anime::Item& anime_item, const std::wstring& root,
                              int episode_number, bool search_folder) {
  if (root.empty())
    return std::wstring();

  anime::Episode episode;
  std::wstring path = AddTrailingSlash(GetExtendedLengthPath(root)) + L"*";

  WIN32_FIND_DATA win32_find_data;
  HANDLE handle = FindFirstFile(path.c_str(), &win32_find_data);

  do {
    if (handle == INVALID_HANDLE_VALUE) {
      DWORD error_code = GetLastError();
      switch (error_code) {
        case ERROR_SUCCESS:
          LOG(LevelError, L"Error code is unavailable.");
          break;
        case ERROR_FILE_NOT_FOUND:
          LOG(LevelError, L"No matching files were found.");
          break;
        default:
          LOG(LevelError, FormatError(error_code));
          break;
      }
      LOG(LevelError, L"Path: " + path);
      SetLastError(ERROR_SUCCESS);
      return std::wstring();
    }

    // Folders
    if (IsDirectory(win32_find_data)) {
      if (IsValidDirectory(win32_find_data)) {
        // Check root folder
        if (search_folder == true) {
          if (Meow.ExamineTitle(win32_find_data.cFileName, episode,
                                false, false, false, false, false)) {
            if (Meow.CompareEpisode(episode, anime_item, true, false, false)) {
              FindClose(handle);
              return AddTrailingSlash(root) + win32_find_data.cFileName;
            }
          }
        }
        // Check sub folders
        path = AddTrailingSlash(root) + win32_find_data.cFileName;
        path = SearchFileFolder(anime_item, path, episode_number,
                                search_folder);
        if (!path.empty()) {
          FindClose(handle);
          return path;
        }
      }

    // Files
    } else {
      if (search_folder == false) {
        // Check file size -- anything less than 10 MB can't be a new episode
        if (win32_find_data.nFileSizeLow > 1024 * 1024 * 10) {
          // Examine file name and extract episode data
          if (Meow.ExamineTitle(win32_find_data.cFileName, episode,
                                true, true, true, true, true)) {
            // Compare episode data with anime title
            if (Meow.CompareEpisode(episode, anime_item)) {
              int number = GetEpisodeHigh(episode.number);
              int numberlow = GetEpisodeLow(episode.number);
              for (int i = numberlow; i <= number; i++) {
                anime_item.SetEpisodeAvailability(
                    i, true, root + win32_find_data.cFileName);
              }
              if (episode_number == 0 ||
                  (episode_number >= numberlow && episode_number <= number)) {
                FindClose(handle);
                return AddTrailingSlash(root) + win32_find_data.cFileName;
              }
            }
          }
        } else {
          LOG(LevelDebug, L"File is ignored because its size does not meet "
                          L"the threshold.");
          LOG(LevelDebug, L"Path: " + AddTrailingSlash(root) +
                          win32_find_data.cFileName);
        }
      }
    }
  } while (FindNextFile(handle, &win32_find_data));
  
  FindClose(handle);
  return std::wstring();
}

////////////////////////////////////////////////////////////////////////////////

void ScanAvailableEpisodes(int anime_id, bool check_folder, bool silent) {
  // Check if any root folder is available
  if (!silent && Settings.root_folders.empty()) {
    ui::OnSettingsRootFoldersEmpty();
    return;
  }

  int episode_number =
      Settings.GetBool(taiga::kApp_List_ProgressDisplayAvailable) ? -1 : 0;

  // Search for all list items
  if (!anime_id) {
    size_t i = 0;
    // Search is made in reverse to give new items priority. The user is
    // probably more interested in them than the older titles.
    if (!silent) {
      TaskbarList.SetProgressState(TBPF_NORMAL);
      SetSharedCursor(IDC_WAIT);
    }
    foreach_r_(it, AnimeDatabase.items) {
      if (!silent)
        TaskbarList.SetProgressValue(i++, AnimeDatabase.items.size());
      switch (it->second.GetMyStatus()) {
        case anime::kWatching:
          if (!silent)
            ui::ChangeStatusText(L"Scanning... (" +
                                 it->second.GetTitle() + L")");
          anime::CheckEpisodes(it->second, episode_number, check_folder);
      }
    }
    i = 0;
    foreach_r_(it, AnimeDatabase.items) {
      if (!silent)
        TaskbarList.SetProgressValue(i++, AnimeDatabase.items.size());
      switch (it->second.GetMyStatus()) {
        case anime::kOnHold:
        case anime::kPlanToWatch:
          if (!silent)
            ui::ChangeStatusText(L"Scanning... (" +
                                 it->second.GetTitle() + L")");
          anime::CheckEpisodes(it->second, episode_number, check_folder);
      }
    }
    if (!silent) {
      TaskbarList.SetProgressState(TBPF_NOPROGRESS);
      SetSharedCursor(IDC_ARROW);
    }

  // Search for a single item
  } else {
    SetSharedCursor(IDC_WAIT);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item)
      anime::CheckEpisodes(*anime_item, episode_number, true);
    SetSharedCursor(IDC_ARROW);
  }

  // We're done
  if (!silent)
    ui::ChangeStatusText(L"Scan finished.");
}