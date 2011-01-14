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

#ifndef MONITOR_H
#define MONITOR_H

#include "std.h"
#include "win32/win_thread.h"

#define WM_MONITORCALLBACK (WM_APP + 0x32)
#define MONITOR_BUFFER_SIZE 4096

// =============================================================================

enum FolderMonitorState {
  MONITOR_STATE_STOPPED,
  MONITOR_STATE_ACTIVE
};

class CFolderInfo {
public:
  CFolderInfo();
  virtual ~CFolderInfo();
  
  vector<wstring> m_ChangeList;
  wstring m_Path;
  int m_State;

  HANDLE m_hDirectory;
  BYTE m_Buffer[MONITOR_BUFFER_SIZE];
  BOOL m_bWatchSubtree;
  DWORD m_dwNotifyFilter;
  DWORD m_dwBytesReturned;
  OVERLAPPED m_Overlapped;
};

// =============================================================================

class CFolderMonitor : public CThread {
public:
  CFolderMonitor();
  virtual ~CFolderMonitor();

  // Worker thread
  DWORD ThreadProc();

  // Main thread
  void Enable(bool enabled = true);
  virtual void OnChange(CFolderInfo* folder, DWORD action);

  // ...
  HANDLE GetCompletionPort() { return m_hCompPort; }
  void SetWindowHandle(HWND hwnd) { m_hWindow = hwnd; }
  
  // ...
  bool AddFolder(const wstring folder);
  bool ClearFolders();
  bool Start();
  bool Stop();

private:
  BOOL ReadDirectoryChanges(CFolderInfo* pfi);

  CCriticalSection m_CriticalSection;
  vector<CFolderInfo> m_Folders;
  HANDLE m_hCompPort;
  HWND m_hWindow;
};

extern CFolderMonitor FolderMonitor;

#endif // MONITOR_H