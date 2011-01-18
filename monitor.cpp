/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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
#include "animelist.h"
#include "common.h"
#include "dlg/dlg_main.h"
#include "monitor.h"
#include "recognition.h"
#include "settings.h"
#include "string.h"

CFolderMonitor FolderMonitor;

// =============================================================================

CFolderInfo::CFolderInfo() :
  m_State(MONITOR_STATE_STOPPED),
  m_hDirectory(INVALID_HANDLE_VALUE),
  m_bWatchSubtree(TRUE),
  m_dwNotifyFilter(FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME),
  m_dwBytesReturned(0)
{
  ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
}

CFolderInfo::~CFolderInfo() {
}

// =============================================================================

CFolderMonitor::CFolderMonitor() :
  m_hCompPort(NULL), m_hWindow(NULL)
{
}

CFolderMonitor::~CFolderMonitor() {
  Stop();
  ClearFolders();
  if (m_hCompPort) ::CloseHandle(m_hCompPort);
}

// =============================================================================

bool CFolderMonitor::AddFolder(const wstring folder) {
  // Validate path
  if (!FolderExists(folder)) {
    return false;
  }

  // Get directory handle
  HANDLE handle = ::CreateFile(folder.c_str(), FILE_LIST_DIRECTORY, 
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, 
    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
  if (handle == INVALID_HANDLE_VALUE) {
    return false;
  }

  // Set folder data
  m_Folders.resize(m_Folders.size() + 1);
  m_Folders.back().m_hDirectory = handle;
  m_Folders.back().m_Path = folder;

  // Success
  return true;
}

bool CFolderMonitor::ClearFolders() {
  // Close handles
  for (unsigned int i = 0; i < m_Folders.size(); i++) {
    if (m_Folders[i].m_hDirectory != INVALID_HANDLE_VALUE) {
      ::CloseHandle(m_Folders[i].m_hDirectory);
      m_Folders[i].m_hDirectory = INVALID_HANDLE_VALUE;
    }
  }
  // Remove all items
  m_Folders.clear();
  return true;
}

bool CFolderMonitor::Start() {
  // Create worker thread
  if (GetThreadHandle() == NULL) {
    CreateThread(NULL, 0, 0);
  }

  // Start watching folders
  if (GetThreadHandle() != NULL) {
    for (unsigned int i = 0; i < m_Folders.size(); i++) {
      m_hCompPort = ::CreateIoCompletionPort(m_Folders[i].m_hDirectory, 
        m_hCompPort, reinterpret_cast<ULONG_PTR>(&m_Folders[i]), 0);
      if (m_hCompPort) {
        ::PostQueuedCompletionStatus(m_hCompPort, sizeof(m_Folders[i]), 
          reinterpret_cast<ULONG_PTR>(&m_Folders[i]), &m_Folders[i].m_Overlapped);
      }
    }
  }

  // Return
  return GetThreadHandle() != NULL;
}

bool CFolderMonitor::Stop() {
  if (GetThreadHandle()) {
    // Signal worker thread to stop
    ::PostQueuedCompletionStatus(m_hCompPort, 0, 0, NULL);
    // Wait for thread to stop
    ::WaitForSingleObject(GetThreadHandle(), INFINITE);
    // Clean up
    CloseThreadHandle();
    ::CloseHandle(m_hCompPort);
    m_hCompPort = NULL;
  }
  return true;
}

// =============================================================================

BOOL CFolderMonitor::ReadDirectoryChanges(CFolderInfo* pfi) {
  return ::ReadDirectoryChangesW(
    pfi->m_hDirectory, pfi->m_Buffer, MONITOR_BUFFER_SIZE, pfi->m_bWatchSubtree, 
    pfi->m_dwNotifyFilter, &pfi->m_dwBytesReturned, &pfi->m_Overlapped, NULL);
}

DWORD CFolderMonitor::ThreadProc() {
  DWORD dwNumBytes = 0;
  CFolderInfo* pfi = NULL;
  LPOVERLAPPED lpOverlapped;

  do {
    // ...
    ::GetQueuedCompletionStatus(GetCompletionPort(), &dwNumBytes, 
      reinterpret_cast<PULONG_PTR>(&pfi), &lpOverlapped, INFINITE);

    if (pfi) {
      // Lock folder data
      m_CriticalSection.Enter();

      switch (pfi->m_State) {
        // Start monitoring
        case MONITOR_STATE_STOPPED: {
          if (ReadDirectoryChanges(pfi)) {
            pfi->m_State = MONITOR_STATE_ACTIVE;
            DEBUG_PRINT(L"CFolderMonitor :: Started monitoring: " + pfi->m_Path + L"\n");
          }
          break;
        }

        // Change detected 
        case MONITOR_STATE_ACTIVE: {
          DWORD dwNextEntryOffset = 0;
          PFILE_NOTIFY_INFORMATION pfni = NULL;
          
          do {
            pfni = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(pfi->m_Buffer + dwNextEntryOffset);
            // Retrieve changed file name
            WCHAR file_name[MAX_PATH + 1] = {'\0'};
            CopyMemory(file_name, pfni->FileName, pfni->FileNameLength);
            DEBUG_PRINT(L"CFolderMonitor :: Change detected: (" + ToWSTR(pfni->Action) + L") " + 
              pfi->m_Path + L"\\" + file_name + L"\n");
            // Add item to list
            pfi->m_ChangeList.resize(pfi->m_ChangeList.size() + 1);
            pfi->m_ChangeList.back().Action = pfni->Action;
            pfi->m_ChangeList.back().FileName = file_name;
            // Continue to next change
            dwNextEntryOffset += pfni->NextEntryOffset;
          } while (pfni->NextEntryOffset != 0);
          
          // Send a message to the main thread
          if (m_hWindow) {
            ::PostMessage(m_hWindow, WM_MONITORCALLBACK, 0, reinterpret_cast<LPARAM>(pfi));
          }
          
          // Continue monitoring
          ReadDirectoryChanges(pfi);
          break;
        }
      }

      // Unlock folder data
      m_CriticalSection.Leave();
    }
  } while (pfi);

  DEBUG_PRINT(L"CFolderMonitor :: Stopped monitoring.\n");
  return 0;
}

// =============================================================================

void CFolderMonitor::Enable(bool enabled) {
  // Stop monitoring
  Stop();
  if (enabled) {
    // Clear folder data
    ClearFolders();
    // Add new folders
    for (unsigned int i = 0; i < Settings.Folders.Root.size(); i++) {
      AddFolder(Settings.Folders.Root[i]);
    }
    // Start monitoring again
    Start();
  }
}

void CFolderMonitor::OnChange(CFolderInfo* info) {
  // Lock folder data
  m_CriticalSection.Enter();

  for (unsigned int i = 0; i < info->m_ChangeList.size(); i++) {
    #define LIST info->m_ChangeList
    
    // Is path available?
    bool path_available;
    switch (info->m_ChangeList[i].Action) {
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
    
    CEpisode episode;
    CheckSlash(info->m_Path);
    wstring path = info->m_Path + LIST[i].FileName;
        
    // Is it a file or a folder?
    bool is_folder = false;
    if (path_available) {
      is_folder = FolderExists(path);
    } else {
      is_folder = !ValidateFileExtension(GetFileExtension(LIST[i].FileName), 4);
    }

    // Compare with list item folders
    if (is_folder && !path_available) {
      int anime_index = 0;
      for (int j = AnimeList.Count; j > 0; j--) {
        if (!AnimeList.Item[j].Folder.empty() && IsEqual(AnimeList.Item[j].Folder, path)) {
          anime_index = j;
          break;
        }
      }
      if (anime_index > 0) {
        if (i == LIST.size() - 1) {
          LIST[i].AnimeIndex = anime_index;
        } else {
          LIST[i + 1].AnimeIndex = anime_index;
          continue;
        }
      }
    }

    // Change anime folder
    if (is_folder && LIST[i].AnimeIndex > 0) {
      #define ANIME AnimeList.Item[LIST[i].AnimeIndex]
      ANIME.Folder = path_available ? path : L"";
      ANIME.CheckEpisodeAvailability();
      Settings.Anime.SetItem(ANIME.Series_ID, EMPTY_STR, ANIME.Folder, EMPTY_STR);
      DEBUG_PRINT(L"CFolderMonitor :: Change anime folder: " + 
        ANIME.Series_Title + L" -> " + ANIME.Folder + L"\n");
      #undef ANIME
      continue;
    }

    // Examine path and compare with list items
    if (Meow.ExamineTitle(path, episode)) {
      if (LIST[i].AnimeIndex == 0 || is_folder == false) {
        for (int j = AnimeList.Count; j > 0; j--) {
          if (Meow.CompareEpisode(episode, AnimeList.Item[j], true, false, false)) {
            LIST[i].AnimeIndex = j;
            break;
          }
        }
      }
      if (LIST[i].AnimeIndex > 0) {
        #define ANIME AnimeList.Item[LIST[i].AnimeIndex]

        // Set anime folder
        if (path_available && ANIME.Folder.empty()) {
          if (is_folder) {
            ANIME.Folder = path;
          } else if (!episode.Folder.empty()) {
            CEpisode temp_episode;
            temp_episode.Title = episode.Folder;
            if (Meow.CompareEpisode(temp_episode, ANIME)) {
              ANIME.Folder = episode.Folder;
            }
          }
          if (!ANIME.Folder.empty()) {
            ANIME.CheckEpisodeAvailability();
            Settings.Anime.SetItem(ANIME.Series_ID, EMPTY_STR, ANIME.Folder, EMPTY_STR);
            DEBUG_PRINT(L"CFolderMonitor :: Set anime folder: " + 
              ANIME.Series_Title + L" -> " + ANIME.Folder + L"\n");
          }
        }

        // Set episode availability
        if (!is_folder) {
          int number = GetLastEpisode(episode.Number);
          if (path_available) {
            path_available = IsEqual(GetPathOnly(path), ANIME.Folder);
          }
          if (ANIME.SetEpisodeAvailability(number, path_available)) {
            DEBUG_PRINT(L"CFolderMonitor :: Episode: (" + ToWSTR(path_available) + L") " + 
              ANIME.Series_Title + L" -> " + ToWSTR(number) + L"\n");
          }
        }
        #undef ANIME
      }
    }
    #undef LIST
  }

  // Clear change list
  info->m_ChangeList.clear();

  // Unlock folder data
  m_CriticalSection.Leave();
}