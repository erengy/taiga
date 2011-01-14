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
  m_dwNotifyFilter(FILE_NOTIFY_CHANGE_FILE_NAME),
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
  DWORD file_attr = ::GetFileAttributes(folder.c_str());
  if (file_attr == INVALID_FILE_ATTRIBUTES || !(file_attr & FILE_ATTRIBUTE_DIRECTORY)) {
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
            pfi->m_ChangeList.push_back(file_name);
            // Send a message to the main thread
            if (m_hWindow) {
              ::PostMessage(m_hWindow, WM_MONITORCALLBACK, pfni->Action, reinterpret_cast<LPARAM>(pfi));
            }
            DEBUG_PRINT(L"CFolderMonitor :: Change detected: " + pfi->m_Path + L"\\" + file_name + L"\n");
            // Continue to next change
            dwNextEntryOffset += pfni->NextEntryOffset;
          } while (pfni->NextEntryOffset != 0);
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

void CFolderMonitor::OnChange(CFolderInfo* folder, DWORD action) {
  // Lock folder data
  m_CriticalSection.Enter();

  // ...
  switch (action) {
    case FILE_ACTION_ADDED:
    case FILE_ACTION_REMOVED:
    case FILE_ACTION_RENAMED_NEW_NAME: {
      for (unsigned int i = 0; i < folder->m_ChangeList.size(); i++) {
        CEpisode episode;
        CheckSlash(folder->m_Path);
        wstring path = folder->m_Path + folder->m_ChangeList[i];
        
        if (Meow.ExamineTitle(path, episode)) {
          for (int i = AnimeList.Count; i > 0; i--) {
            if (Meow.CompareEpisode(episode, AnimeList.Item[i])) {
              #define ANIME AnimeList.Item[i]
              
              // Set anime folder
              if (ANIME.Folder.empty() && !episode.Folder.empty()) {
                CEpisode temp_episode; temp_episode.Title = episode.Folder;
                if (Meow.CompareEpisode(temp_episode, ANIME)) {
                  ANIME.Folder = episode.Folder;
                  Settings.Anime.SetItem(ANIME.Series_ID, EMPTY_STR, ANIME.Folder, EMPTY_STR);
                  DEBUG_PRINT(L"CFolderMonitor :: Set anime folder: " + ANIME.Series_Title + 
                    L" -> " + ANIME.Folder + L"\n");
                }
              }

              // Set episode availability
              int number = GetLastEpisode(episode.Number);
              if (ANIME.SetEpisodeAvailability(number, action != FILE_ACTION_REMOVED)) {
                DEBUG_PRINT(L"CFolderMonitor :: New episode: " + ANIME.Series_Title + 
                  L" -> " + ToWSTR(number) + L" (" + ToWSTR(action != FILE_ACTION_REMOVED) + L")\n");
              }

              #undef ANIME
              break;
            }
          }
        }
      }

      folder->m_ChangeList.clear();
      break;
    }
  }

  // Unlock folder data
  m_CriticalSection.Leave();
}