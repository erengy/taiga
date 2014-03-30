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

#include "base/file.h"
#include "base/foreach.h"
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

FolderInfo::FolderInfo()
    : bytes_returned_(0),
      directory_handle_(INVALID_HANDLE_VALUE),
      notify_filter_(FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME),
      state(kFolderMonitorStateStopped),
      watch_subtree_(TRUE) {
  ZeroMemory(&overlapped_, sizeof(overlapped_));
}

FolderInfo::FolderChangeInfo::FolderChangeInfo()
    : action(0), parameter(0), type(kPathTypeFile) {
}

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

bool FolderMonitor::AddFolder(const std::wstring& folder) {
  if (!FolderExists(folder))
    return false;

  HANDLE handle = ::CreateFile(
      folder.c_str(),
      FILE_LIST_DIRECTORY,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      nullptr,
      OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
      nullptr);

  if (handle == INVALID_HANDLE_VALUE)
    return false;

  folders_.resize(folders_.size() + 1);
  folders_.back().directory_handle_ = handle;
  folders_.back().path = folder;

  return true;
}

bool FolderMonitor::ClearFolders() {
  // Close handles
  foreach_(folder, folders_) {
    if (folder->directory_handle_ != INVALID_HANDLE_VALUE) {
      ::CloseHandle(folder->directory_handle_);
      folder->directory_handle_ = INVALID_HANDLE_VALUE;
    }
  }

  folders_.clear();

  return true;
}

bool FolderMonitor::Start() {
  // Create worker thread
  if (!GetThreadHandle())
    CreateThread(nullptr, 0, 0);

  // Start watching folders
  if (GetThreadHandle()) {
    foreach_(folder, folders_) {
      completion_port_ = ::CreateIoCompletionPort(
          folder->directory_handle_,
          completion_port_,
          reinterpret_cast<ULONG_PTR>(&(*folder)),
          0);
      if (completion_port_)
        ::PostQueuedCompletionStatus(
            completion_port_,
            sizeof(*folder),
            reinterpret_cast<ULONG_PTR>(&(*folder)),
            &folder->overlapped_);
    }
  }

  return GetThreadHandle() != nullptr;
}

void FolderMonitor::Stop() {
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
}

////////////////////////////////////////////////////////////////////////////////

BOOL FolderMonitor::ReadDirectoryChanges(FolderInfo& folder_info) const {
  return ::ReadDirectoryChangesW(folder_info.directory_handle_,
                                 folder_info.buffer_,
                                 MONITOR_BUFFER_SIZE,
                                 folder_info.watch_subtree_,
                                 folder_info.notify_filter_,
                                 &folder_info.bytes_returned_,
                                 &folder_info.overlapped_,
                                 nullptr);
}

DWORD FolderMonitor::ThreadProc() {
  DWORD dwNumBytes = 0;
  FolderInfo* folder_info = nullptr;
  LPOVERLAPPED lpOverlapped;

  do {
    ::GetQueuedCompletionStatus(GetCompletionPort(),
                                &dwNumBytes,
                                reinterpret_cast<PULONG_PTR>(&folder_info),
                                &lpOverlapped,
                                INFINITE);

    if (folder_info) {
      // Lock folder data
      win::Lock lock(critical_section_);

      switch (folder_info->state) {
        // Start monitoring
        case kFolderMonitorStateStopped: {
          if (ReadDirectoryChanges(*folder_info)) {
            folder_info->state = kFolderMonitorStateActive;
            LOG(LevelDebug, L"Started monitoring: " + folder_info->path);
          }
          break;
        }

        // Change detected 
        case kFolderMonitorStateActive: {
          DWORD dwNextEntryOffset = 0;
          PFILE_NOTIFY_INFORMATION pfni = nullptr;

          do {
            pfni = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(
                folder_info->buffer_ + dwNextEntryOffset);
            // Retrieve changed file name
            WCHAR file_name[MAX_PATH + 1] = {'\0'};
            CopyMemory(file_name, pfni->FileName, pfni->FileNameLength);
            // Add item to list
            folder_info->change_list.resize(folder_info->change_list.size() + 1);
            folder_info->change_list.back().action = pfni->Action;
            folder_info->change_list.back().file_name = file_name;
            // Continue to next change
            dwNextEntryOffset += pfni->NextEntryOffset;
          } while (pfni->NextEntryOffset != 0);

          // Post a message to the main thread
          if (window_handle_)
            ::PostMessage(window_handle_, WM_MONITORCALLBACK, 0,
                          reinterpret_cast<LPARAM>(folder_info));

          // Continue monitoring
          ReadDirectoryChanges(*folder_info);
          break;
        }
      }
    }
  } while (folder_info);

  LOG(LevelDebug, L"Stopped monitoring.");

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

void FolderMonitor::Enable(bool enabled) {
  Stop();

  if (enabled) {
    ClearFolders();

    foreach_(folder, Settings.root_folders)
      AddFolder(*folder);

    Start();
  }
}

HANDLE FolderMonitor::GetCompletionPort() const {
  return completion_port_;
}

void FolderMonitor::SetWindowHandle(HWND hwnd) {
  window_handle_ = hwnd;
}

////////////////////////////////////////////////////////////////////////////////

bool FolderMonitor::IsPathAvailable(DWORD action) const {
  switch (action) {
    case FILE_ACTION_ADDED:
    case FILE_ACTION_RENAMED_NEW_NAME:
      return true;
    case FILE_ACTION_REMOVED:
    case FILE_ACTION_RENAMED_OLD_NAME:
    default:
      return false;
  }
}

void FolderMonitor::OnChange(FolderInfo& folder_info) {
  // Lock folder data
  win::Lock lock(critical_section_);

  foreach_(change_info, folder_info.change_list) {
    switch (change_info->action) {
      case FILE_ACTION_ADDED:
      case FILE_ACTION_REMOVED:
      case FILE_ACTION_RENAMED_OLD_NAME:
      case FILE_ACTION_RENAMED_NEW_NAME:
        break;
      default:
        continue;
    }

    AddTrailingSlash(folder_info.path);
    std::wstring path = folder_info.path + change_info->file_name;

    // Is it a file or a directory?
    if (IsPathAvailable(change_info->action)) {
      if (FolderExists(path))
        change_info->type = kPathTypeDirectory;
    } else {
      std::wstring file_extension = GetFileExtension(change_info->file_name);
      if (!ValidateFileExtension(file_extension, 4))
        change_info->type = kPathTypeDirectory;
    }

    switch (change_info->action) {
      case FILE_ACTION_ADDED:
        LOG(LevelDebug, L"Added: " + path);
        break;
      case FILE_ACTION_REMOVED:
        LOG(LevelDebug, L"Removed: " + path);
        break;
      case FILE_ACTION_RENAMED_OLD_NAME:
        LOG(LevelDebug, L"Renamed (old): " + path);
        break;
      case FILE_ACTION_RENAMED_NEW_NAME:
        LOG(LevelDebug, L"Renamed (new): " + path);
        break;
    }

    size_t change_index = change_info - folder_info.change_list.begin();
    HandleAnime(path, folder_info, change_index);
  }

  // Clear change list
  folder_info.change_list.clear();
}

void ChangeAnimeFolder(anime::Item& anime_item, const std::wstring& path) {
  anime_item.SetFolder(path);
  Settings.Save();

  LOG(LevelDebug, L"Anime folder changed: " + anime_item.GetTitle());
  LOG(LevelDebug, L"Path: " + anime_item.GetFolder());

  ScanAvailableEpisodesQuick(anime_item.GetId());
}

void FolderMonitor::HandleAnime(const std::wstring& path,
                                FolderInfo& folder_info,
                                size_t change_index) {
  auto& change_info = folder_info.change_list.at(change_index);
  bool path_available = IsPathAvailable(change_info.action);

  int anime_id = anime::ID_UNKNOWN;
  if (change_info.parameter)
    anime_id = change_info.parameter;

  if (change_info.type == kPathTypeDirectory) {
    // Compare with list item folders
    if (!path_available) {
      foreach_cr_(it, AnimeDatabase.items) {
        if (!it->second.IsInList())
          continue;
        if (!it->second.GetFolder().empty() &&
            IsEqual(it->second.GetFolder(), path)) {
          anime_id = it->second.GetId();
          break;
        }
      }
      if (anime_id != anime::ID_UNKNOWN) {
        // Handle next change
        if (change_index < folder_info.change_list.size() - 1) {
          folder_info.change_list.at(change_index + 1).parameter = anime_id;
          return;
        }
      }
    }

    if (anime_id != anime::ID_UNKNOWN) {
      // Change anime folder
      auto anime_item = AnimeDatabase.FindItem(anime_id);
      ChangeAnimeFolder(*anime_item, path_available ? path : L"");
      return;
    }
  }

  // Examine path and compare with list items
  anime::Episode episode;
  if (Meow.ExamineTitle(path, episode)) {
    if (anime_id == anime::ID_UNKNOWN || change_info.type == kPathTypeFile) {
      auto anime_item = Meow.MatchDatabase(episode, true, true, true, false, false);
      if (anime_item)
        anime_id = anime_item->GetId();
    }

    if (anime_id != anime::ID_UNKNOWN) {
      auto anime_item = AnimeDatabase.FindItem(anime_id);

      // Set anime folder
      if (path_available && anime_item->GetFolder().empty()) {
        if (change_info.type == kPathTypeDirectory) {
          ChangeAnimeFolder(*anime_item, path);
        } else if (!episode.folder.empty()) {
          anime::Episode temp_episode;
          temp_episode.title = episode.folder;
          if (Meow.CompareEpisode(temp_episode, *anime_item))
            ChangeAnimeFolder(*anime_item, episode.folder);
        }
      }

      // Set episode availability
      if (change_info.type == kPathTypeFile) {
        int number = anime::GetEpisodeHigh(episode.number);
        int number_low = anime::GetEpisodeLow(episode.number);
        for (int j = number_low; j <= number; j++) {
          if (anime_item->SetEpisodeAvailability(number, path_available, path)) {
            LOG(LevelDebug, anime_item->GetTitle() + L" #" + ToWstr(j) + L" is " +
                            (path_available ? L"available." : L"unavailable."));
          }
        }
      }
    }
  }
}