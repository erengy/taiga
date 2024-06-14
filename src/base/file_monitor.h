/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <windows.h>
#include <string>
#include <vector>

#include <windows/win/thread.h>

constexpr unsigned int WM_MONITORCALLBACK = WM_APP + 0x32;

class DirectoryChangeNotification {
public:
  enum class Type {
    Directory,
    File,
    Unknown,
  };

  DirectoryChangeNotification(DWORD action, const std::wstring& filename,
                              const std::wstring& path);

  DWORD action;
  std::pair<std::wstring, std::wstring> filename;
  std::wstring path;
  Type type;
};

class DirectoryChangeEntry {
public:
  friend class DirectoryMonitor;

  enum class State {
    Stopped,
    Active,
  };

  DirectoryChangeEntry(HANDLE directory_handle, const std::wstring& path);

  std::vector<DirectoryChangeNotification> notifications;
  std::wstring path;
  State state;

private:
  std::vector<BYTE> buffer_;
  DWORD bytes_returned_;
  HANDLE directory_handle_;
  OVERLAPPED overlapped_;
};

////////////////////////////////////////////////////////////////////////////////
// Monitors the contents of a directory and its subdirectories by using change
// notifications.
class DirectoryMonitor {
public:
  DirectoryMonitor();
  virtual ~DirectoryMonitor();

  // The window must handle WM_MONITORCALLBACK message and call the callback
  // function. lParam of the message is a pointer to a DirectoryChangeEntry.
  void Callback(DirectoryChangeEntry& entry);
  void SetWindowHandle(HWND hwnd);

  // Override this function to handle notifications
  virtual void HandleChangeNotification(
      const DirectoryChangeNotification& notification) const = 0;

protected:
  bool Add(const std::wstring& path);
  void Clear();

  bool Start();
  void Stop();

private:
  bool ReadDirectoryChanges(DirectoryChangeEntry& entry);
  void MonitorProc();
  void HandleStoppedState(DirectoryChangeEntry& entry);
  void HandleActiveState(DirectoryChangeEntry& entry);

  class Thread : public win::Thread {
  public:
    DWORD ThreadProc();
    DirectoryMonitor* parent;
  } thread_;

  std::vector<DirectoryChangeEntry> entries_;
  win::CriticalSection critical_section_;
  HANDLE completion_port_;
  HWND window_handle_;
};
