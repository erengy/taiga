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

#ifndef TAIGA_TRACK_MONITOR_H
#define TAIGA_TRACK_MONITOR_H

#include <string>
#include <vector>

#include "win/win_thread.h"

#define WM_MONITORCALLBACK (WM_APP + 0x32)
#define MONITOR_BUFFER_SIZE 4096

class FolderMonitor;

enum FolderMonitorState {
  kFolderMonitorStateStopped,
  kFolderMonitorStateActive
};

enum PathType {
  kPathTypeFile,
  kPathTypeDirectory
};

class FolderInfo {
public:
  FolderInfo();
  ~FolderInfo() {}

  friend class FolderMonitor;

  std::wstring path;
  int state;

  class FolderChangeInfo {
  public:
    FolderChangeInfo();
    DWORD action;
    std::wstring file_name;
    LPARAM parameter;
    PathType type;
  };
  std::vector<FolderChangeInfo> change_list;

private:
  BYTE buffer_[MONITOR_BUFFER_SIZE];
  DWORD bytes_returned_;
  HANDLE directory_handle_;
  DWORD notify_filter_;
  OVERLAPPED overlapped_;
  BOOL watch_subtree_;
};

class FolderMonitor : public win::Thread {
public:
  FolderMonitor();
  virtual ~FolderMonitor();

  // Worker thread
  DWORD ThreadProc();

  // Main thread
  void Enable(bool enabled = true);
  virtual void OnChange(FolderInfo& folder_info);

  HANDLE GetCompletionPort() const;
  void SetWindowHandle(HWND hwnd);

  bool AddFolder(const std::wstring& folder);
  bool ClearFolders();
  bool Start();
  void Stop();

private:
  void HandleAnime(const std::wstring& path, FolderInfo& folder_info, size_t change_index);
  bool IsPathAvailable(DWORD action) const;
  BOOL ReadDirectoryChanges(FolderInfo& folder_info) const;

  win::CriticalSection critical_section_;
  std::vector<FolderInfo> folders_;
  HANDLE completion_port_;
  HWND window_handle_;
};

extern FolderMonitor FolderMonitor;

#endif  // TAIGA_TRACK_MONITOR_H