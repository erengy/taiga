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

#ifndef MONITOR_H
#define MONITOR_H

#include "std.h"
#include "win32/win_thread.h"

#define WM_MONITORCALLBACK (WM_APP + 0x32)
#define MONITOR_BUFFER_SIZE 4096

class FolderMonitor;

// =============================================================================

enum FolderMonitorState {
  MONITOR_STATE_STOPPED,
  MONITOR_STATE_ACTIVE
};

class FolderInfo {
public:
  FolderInfo();
  virtual ~FolderInfo();

  friend class FolderMonitor;
  
public:
  wstring path;
  int state;
  
  class FolderChangeInfo {
  public:
    FolderChangeInfo() : anime_index(0) {}
    DWORD action;
    int anime_index;
    wstring file_name;
  };
  vector<FolderChangeInfo> change_list;
  
private:
  BYTE buffer_[MONITOR_BUFFER_SIZE];
  DWORD bytes_returned_;
  HANDLE directory_handle_;
  DWORD notify_filter_;
  OVERLAPPED overlapped_;
  BOOL watch_subtree_;
};

// =============================================================================

class FolderMonitor : public CThread {
public:
  FolderMonitor();
  virtual ~FolderMonitor();

  // Worker thread
  DWORD ThreadProc();

  // Main thread
  void Enable(bool enabled = true);
  virtual void OnChange(FolderInfo* folder_info);

  HANDLE GetCompletionPort() { return completion_port_; }
  void SetWindowHandle(HWND hwnd) { window_handle_ = hwnd; }

  bool AddFolder(const wstring& folder);
  bool ClearFolders();
  bool Start();
  bool Stop();

private:
  BOOL ReadDirectoryChanges(FolderInfo* folder_info);

private:
  CCriticalSection critical_section_;
  vector<FolderInfo> folders_;
  HANDLE completion_port_;
  HWND window_handle_;
};

extern FolderMonitor FolderMonitor;

#endif // MONITOR_H