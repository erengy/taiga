/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#include "base/std.h"

#include "library/anime.h"
#include "library/anime_db.h"
#include "library/anime_episode.h"
#include "library/anime_item.h"
#include "base/common.h"
#include "base/foreach.h"
#include "base/logger.h"
#include "recognition.h"
#include "taiga/settings.h"
#include "base/string.h"
#include "taiga/taiga.h"

#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_settings.h"

#include "win/win_taskbar.h"
#include "win/win_taskdialog.h"

// =============================================================================

// Extends the length limit from 260 to 32767 characters
wstring GetExtendedLengthPath(const wstring& path) {
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

wstring SearchFileFolder(anime::Item& anime_item, const wstring& root,
                         int episode_number, bool search_folder) {
  if (root.empty())
    return wstring();

  anime::Episode episode;
  wstring path = AddTrailingSlash(GetExtendedLengthPath(root)) + L"*";

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
      return wstring();
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
        path = SearchFileFolder(anime_item, path, episode_number, search_folder);
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
                anime_item.SetEpisodeAvailability(i, true, root + win32_find_data.cFileName);
              }
              if (episode_number == 0 || (episode_number >= numberlow && episode_number <= number)) {
                FindClose(handle);
                return AddTrailingSlash(root) + win32_find_data.cFileName;
              }
            }
          }
        } else {
          LOG(LevelDebug, L"File is ignored because its size does not meet the threshold.");
          LOG(LevelDebug, L"Path: " + AddTrailingSlash(root) + win32_find_data.cFileName);
        }
      }
    }
  } while (FindNextFile(handle, &win32_find_data));
  
  FindClose(handle);
  return wstring();
}

// =============================================================================

void ScanAvailableEpisodes(int anime_id, bool check_folder, bool silent) {
  // Check if any root folder is available
  if (!silent && Settings.Folders.root.empty()) {
    win::TaskDialog dlg(APP_TITLE, TD_ICON_INFORMATION);
    dlg.SetMainInstruction(L"Would you like to set root anime folders first?");
    dlg.SetContent(L"You need to have at least one root folder set before scanning available episodes.");
    dlg.AddButton(L"Yes", IDYES);
    dlg.AddButton(L"No", IDNO);
    dlg.Show(g_hMain);
    if (dlg.GetSelectedButtonID() == IDYES)
      ExecuteAction(L"Settings", SECTION_LIBRARY, PAGE_LIBRARY_FOLDERS);
    return;
  }

  int episode_number = Settings.Program.List.progress_show_available ? -1 : 0;

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
            MainDialog.ChangeStatus(L"Scanning... (" + it->second.GetTitle() + L")");
          it->second.CheckEpisodes(episode_number, check_folder);
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
            MainDialog.ChangeStatus(L"Scanning... (" + it->second.GetTitle() + L")");
          it->second.CheckEpisodes(episode_number, check_folder);
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
      anime_item->CheckEpisodes(episode_number, true);
    SetSharedCursor(IDC_ARROW);
  }

  // We're done
  if (!silent)
    MainDialog.ChangeStatus(L"Scan finished.");
}