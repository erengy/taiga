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

#include "std.h"

#include "monitor.h"

#include "anime_db.h"
#include "anime_episode.h"
#include "common.h"
#include "debug.h"
#include "recognition.h"
#include "settings.h"
#include "string.h"

#include "dlg/dlg_main.h"

class FolderMonitor FolderMonitor;

// =============================================================================

FolderInfo::FolderInfo()
    : bytes_returned_(0),
      directory_handle_(INVALID_HANDLE_VALUE),
      notify_filter_(FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME),
      state(MONITOR_STATE_STOPPED),
      watch_subtree_(TRUE) {
  ZeroMemory(&overlapped_, sizeof(overlapped_));
}

FolderInfo::~FolderInfo() {
}

// =============================================================================

FolderMonitor::FolderMonitor()
    : completion_port_(nullptr), 
      window_handle_(nullptr) {
}

FolderMonitor::~FolderMonitor() {
  Stop();
  ClearFolders();
  if (completion_port_) {
    ::CloseHandle(completion_port_);
    completion_port_ = nullptr;
  }
}

// =============================================================================

bool FolderMonitor::AddFolder(const wstring& folder) {
  // Validate path
  if (!FolderExists(folder)) {
    return false;
  }

  // Get directory handle
  HANDLE handle = ::CreateFile(folder.c_str(), FILE_LIST_DIRECTORY, 
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, 
    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    return false;
  }

  // Set folder data
  folders_.resize(folders_.size() + 1);
  folders_.back().directory_handle_ = handle;
  folders_.back().path = folder;

  // Success
  return true;
}

bool FolderMonitor::ClearFolders() {
  // Close handles
  for (unsigned int i = 0; i < folders_.size(); i++) {
    if (folders_[i].directory_handle_ != INVALID_HANDLE_VALUE) {
      ::CloseHandle(folders_[i].directory_handle_);
      folders_[i].directory_handle_ = INVALID_HANDLE_VALUE;
    }
  }
  // Remove all items
  folders_.clear();
  return true;
}

bool FolderMonitor::Start() {
  // Create worker thread
  if (GetThreadHandle() == nullptr) {
    CreateThread(nullptr, 0, 0);
  }

  // Start watching folders
  if (GetThreadHandle() != nullptr) {
    for (unsigned int i = 0; i < folders_.size(); i++) {
      completion_port_ = ::CreateIoCompletionPort(folders_[i].directory_handle_, 
        completion_port_, reinterpret_cast<ULONG_PTR>(&folders_[i]), 0);
      if (completion_port_) {
        ::PostQueuedCompletionStatus(completion_port_, sizeof(folders_[i]), 
          reinterpret_cast<ULONG_PTR>(&folders_[i]), &folders_[i].overlapped_);
      }
    }
  }

  // Return
  return GetThreadHandle() != nullptr;
}

bool FolderMonitor::Stop() {
  if (GetThreadHandle()) {
    // Signal worker thread to stop
    ::PostQueuedCompletionStatus(completion_port_, 0, 0, nullptr);
    // Wait for thread to stop
    ::WaitForSingleObject(GetThreadHandle(), INFINITE);
    // Clean up
    CloseThreadHandle();
    ::CloseHandle(completion_port_);
    completion_port_ = nullptr;
  }
  return true;
}

// =============================================================================

BOOL FolderMonitor::ReadDirectoryChanges(FolderInfo* folder_info) {
  return ::ReadDirectoryChangesW(
    folder_info->directory_handle_, 
    folder_info->buffer_, 
    MONITOR_BUFFER_SIZE, 
    folder_info->watch_subtree_, 
    folder_info->notify_filter_, 
    &folder_info->bytes_returned_, 
    &folder_info->overlapped_, 
    nullptr);
}

DWORD FolderMonitor::ThreadProc() {
  DWORD dwNumBytes = 0;
  FolderInfo* folder_info = nullptr;
  LPOVERLAPPED lpOverlapped;

  do {
    // ...
    ::GetQueuedCompletionStatus(GetCompletionPort(), &dwNumBytes, 
      reinterpret_cast<PULONG_PTR>(&folder_info), &lpOverlapped, INFINITE);

    if (folder_info) {
      // Lock folder data
      critical_section_.Enter();

      switch (folder_info->state) {
        // Start monitoring
        case MONITOR_STATE_STOPPED: {
          if (ReadDirectoryChanges(folder_info)) {
            folder_info->state = MONITOR_STATE_ACTIVE;
            debug::Print(L"FolderMonitor :: Started monitoring: " + folder_info->path + L"\n");
          }
          break;
        }

        // Change detected 
        case MONITOR_STATE_ACTIVE: {
          DWORD dwNextEntryOffset = 0;
          PFILE_NOTIFY_INFORMATION pfni = nullptr;
          
          do {
            pfni = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(folder_info->buffer_ + dwNextEntryOffset);
            // Retrieve changed file name
            WCHAR file_name[MAX_PATH + 1] = {'\0'};
            CopyMemory(file_name, pfni->FileName, pfni->FileNameLength);
            debug::Print(L"FolderMonitor :: Change detected: (" + ToWstr(pfni->Action) + L") " + 
              folder_info->path + L"\\" + file_name + L"\n");
            // Add item to list
            folder_info->change_list.resize(folder_info->change_list.size() + 1);
            folder_info->change_list.back().action = pfni->Action;
            folder_info->change_list.back().file_name = file_name;
            // Continue to next change
            dwNextEntryOffset += pfni->NextEntryOffset;
          } while (pfni->NextEntryOffset != 0);
          
          // Send a message to the main thread
          if (window_handle_) {
            ::PostMessage(window_handle_, WM_MONITORCALLBACK, 0, reinterpret_cast<LPARAM>(folder_info));
          }
          
          // Continue monitoring
          ReadDirectoryChanges(folder_info);
          break;
        }
      }

      // Unlock folder data
      critical_section_.Leave();
    }
  } while (folder_info);

  debug::Print(L"FolderMonitor :: Stopped monitoring.\n");
  return 0;
}

// =============================================================================

void FolderMonitor::Enable(bool enabled) {
  // Stop monitoring
  Stop();
  if (enabled) {
    // Clear folder data
    ClearFolders();
    // Add new folders
    for (unsigned int i = 0; i < Settings.Folders.root.size(); i++) {
      AddFolder(Settings.Folders.root[i]);
    }
    // Start monitoring again
    Start();
  }
}

void FolderMonitor::OnChange(FolderInfo* folder_info) {
  // Lock folder data
  critical_section_.Enter();

  for (unsigned int i = 0; i < folder_info->change_list.size(); i++) {
    #define LIST folder_info->change_list
    
    // Is path available?
    bool path_available;
    switch (folder_info->change_list[i].action) {
      case FILE_ACTION_ADDED:
      case FILE_ACTION_RENAMED_NEW_NAME:
        path_available = true;
        break;
      case FILE_ACTION_REMOVED:
      case FILE_ACTION_RENAMED_OLD_NAME:
        path_available = false;
        break;
      default:
        continue;
    }
    
    anime::Episode episode;
    CheckSlash(folder_info->path);
    wstring path = folder_info->path + LIST[i].file_name;
        
    // Is it a file or a folder?
    bool is_folder = false;
    if (path_available) {
      is_folder = FolderExists(path);
    } else {
      is_folder = !ValidateFileExtension(GetFileExtension(LIST[i].file_name), 4);
    }

    // Compare with list item folders
    if (is_folder && !path_available) {
      int anime_id = 0;
      for (auto it = AnimeDatabase.items.rbegin(); it != AnimeDatabase.items.rend(); ++it) {
        if (!it->IsInList()) continue;
        if (!it->GetFolder().empty() && IsEqual(it->GetFolder(), path)) {
          anime_id = it->GetId();
          break;
        }
      }
      if (anime_id > 0) {
        if (i == LIST.size() - 1) {
          LIST[i].anime_id = anime_id;
        } else {
          LIST[i + 1].anime_id = anime_id;
          continue;
        }
      }
    }

    // Change anime folder
    if (is_folder && LIST[i].anime_id > 0) {
      auto anime_item = AnimeDatabase.FindItem(LIST[i].anime_id);
      anime_item->SetFolder(path_available ? path : L"", true);
      anime_item->CheckEpisodes();
      debug::Print(L"FolderMonitor :: Change anime folder: " + 
        anime_item->GetTitle() + L" -> " + anime_item->GetFolder() + L"\n");
      continue;
    }

    // Examine path and compare with list items
    if (Meow.ExamineTitle(path, episode)) {
      if (LIST[i].anime_id == 0 || is_folder == false) {
        for (auto it = AnimeDatabase.items.rbegin(); it != AnimeDatabase.items.rend(); ++it) {
          if (!it->IsInList()) continue;
          if (Meow.CompareEpisode(episode, *it, true, false, false)) {
            LIST[i].anime_id = it->GetId();
            break;
          }
        }
      }
      if (LIST[i].anime_id > 0) {
        auto anime_item = AnimeDatabase.FindItem(LIST[i].anime_id);

        // Set anime folder
        if (path_available && anime_item->GetFolder().empty()) {
          if (is_folder) {
            anime_item->SetFolder(path, true);
          } else if (!episode.folder.empty()) {
            anime::Episode temp_episode;
            temp_episode.title = episode.folder;
            if (Meow.CompareEpisode(temp_episode, *anime_item)) {
              anime_item->SetFolder(episode.folder, true);
              anime_item->CheckEpisodes();
            }
          }
          if (!anime_item->GetFolder().empty()) {
            debug::Print(L"FolderMonitor :: Set anime folder: " + 
              anime_item->GetTitle() + L" -> " + anime_item->GetFolder() + L"\n");
          }
        }

        // Set episode availability
        if (!is_folder) {
          int number = GetEpisodeHigh(episode.number);
          int numberlow = GetEpisodeLow(episode.number);
          if (path_available) {
            path_available = IsEqual(GetPathOnly(path), anime_item->GetFolder());
          }
          for (int j = numberlow; j <= number; j++) {
            if (anime_item->SetEpisodeAvailability(number, path_available, path)) {
              debug::Print(L"FolderMonitor :: Episode: (" + ToWstr(path_available) + L") " + 
                anime_item->GetTitle() + L" -> " + ToWstr(j) + L"\n");
            }
          }
        }
      }
    }
    #undef LIST
  }

  // Clear change list
  folder_info->change_list.clear();

  // Unlock folder data
  critical_section_.Leave();
}