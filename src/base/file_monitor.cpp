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

#include "base/file_monitor.h"

#include "base/file.h"
#include "base/log.h"
#include "base/string.h"

DirectoryChangeNotification::DirectoryChangeNotification(
    DWORD action, const std::wstring& filename, const std::wstring& path)
    : action(action),
      filename(std::make_pair(filename, L"")),
      path(path),
      type(Type::Unknown) {
}

////////////////////////////////////////////////////////////////////////////////

DirectoryChangeEntry::DirectoryChangeEntry(HANDLE directory_handle,
                                           const std::wstring& path)
    : bytes_returned_(0),
      directory_handle_(directory_handle),
      path(path),
      state(State::Stopped) {
  buffer_.resize(65536);
  ZeroMemory(&overlapped_, sizeof(overlapped_));
}

////////////////////////////////////////////////////////////////////////////////

DirectoryMonitor::DirectoryMonitor()
    : completion_port_(nullptr), window_handle_(nullptr) {
  thread_.parent = this;
}

DirectoryMonitor::~DirectoryMonitor() {
  Stop();
  Clear();
}

void DirectoryMonitor::SetWindowHandle(HWND hwnd) { window_handle_ = hwnd; }

////////////////////////////////////////////////////////////////////////////////

bool DirectoryMonitor::Add(const std::wstring& path) {
  if (!FolderExists(path))
    return false;

  HANDLE directory_handle = ::CreateFile(
      path.c_str(),
      FILE_LIST_DIRECTORY,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      nullptr,
      OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
      nullptr);

  if (directory_handle == INVALID_HANDLE_VALUE)
    return false;

  entries_.push_back(DirectoryChangeEntry(directory_handle, path));
  AddTrailingSlash(entries_.back().path);

  return true;
}

void DirectoryMonitor::Clear() {
  for (auto& entry : entries_) {
    if (entry.directory_handle_ != INVALID_HANDLE_VALUE) {
      ::CloseHandle(entry.directory_handle_);
      entry.directory_handle_ = INVALID_HANDLE_VALUE;
    }
  }

  entries_.clear();
}

////////////////////////////////////////////////////////////////////////////////

bool DirectoryMonitor::Start() {
  if (!thread_.GetThreadHandle())
    thread_.CreateThread(nullptr, 0, 0);

  if (!thread_.GetThreadHandle())
    return false;

  for (auto& entry : entries_) {
    auto completion_key = reinterpret_cast<ULONG_PTR>(&entry);
    completion_port_ = ::CreateIoCompletionPort(
        entry.directory_handle_, completion_port_, completion_key, 0);
    if (completion_port_)
      ::PostQueuedCompletionStatus(completion_port_, sizeof(entry),
                                   completion_key, &entry.overlapped_);
  }

  return true;
}

void DirectoryMonitor::Stop() {
  if (thread_.GetThreadHandle()) {
    ::PostQueuedCompletionStatus(completion_port_, 0, 0, nullptr);
    ::WaitForSingleObject(thread_.GetThreadHandle(), INFINITE);
    thread_.CloseThreadHandle();
  }

  if (completion_port_) {
    ::CloseHandle(completion_port_);
    completion_port_ = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////

bool DirectoryMonitor::ReadDirectoryChanges(DirectoryChangeEntry& entry) {
  const auto result = ::ReadDirectoryChangesW(
      entry.directory_handle_,
      entry.buffer_.data(),
      static_cast<DWORD>(entry.buffer_.size()),
      TRUE,  // watch subtree
      FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME,
      &entry.bytes_returned_,
      &entry.overlapped_,
      nullptr);

  return result != 0;
}

DWORD DirectoryMonitor::Thread::ThreadProc() {
  parent->MonitorProc();
  return 0;
}

void DirectoryMonitor::MonitorProc() {
  DirectoryChangeEntry* entry = nullptr;
  DWORD number_of_bytes = 0;
  LPOVERLAPPED overlapped;

  do {
    ::GetQueuedCompletionStatus(completion_port_, &number_of_bytes,
                                reinterpret_cast<PULONG_PTR>(&entry),
                                &overlapped, INFINITE);

    if (entry && number_of_bytes > 0) {
      win::Lock lock(critical_section_);
      switch (entry->state) {
        case DirectoryChangeEntry::State::Stopped: {
          HandleStoppedState(*entry);
          break;
        }
        case DirectoryChangeEntry::State::Active: {
          HandleActiveState(*entry);
          break;
        }
      }
    }
  } while (entry);

  LOGD(L"Stopped monitoring.");
}

void DirectoryMonitor::HandleStoppedState(DirectoryChangeEntry& entry) {
  if (ReadDirectoryChanges(entry)) {
    entry.state = DirectoryChangeEntry::State::Active;
    LOGD(L"Started monitoring: {}", entry.path);
  }
}

void DirectoryMonitor::HandleActiveState(DirectoryChangeEntry& entry) {
  DWORD next_entry_offset = 0;
  PFILE_NOTIFY_INFORMATION file_notify_info = nullptr;

  do {
    file_notify_info = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(
        entry.buffer_.data() + next_entry_offset);
    // Retrieve filename
    size_t length = file_notify_info->FileNameLength / sizeof(wchar_t);
    std::wstring filename(file_notify_info->FileName, length);
    // Create a new notification
    entry.notifications.push_back(DirectoryChangeNotification(
        file_notify_info->Action, filename, entry.path));
    // Continue to the next entry
    next_entry_offset += file_notify_info->NextEntryOffset;
  } while (file_notify_info->NextEntryOffset != 0);

  // Post a message to the main thread
  if (window_handle_) {
    ::PostMessage(window_handle_, WM_MONITORCALLBACK, 0,
                  reinterpret_cast<LPARAM>(&entry));
  }

  // Continue monitoring
  ReadDirectoryChanges(entry);
}

////////////////////////////////////////////////////////////////////////////////

static void LogFileAction(const DirectoryChangeEntry& entry,
                          const DirectoryChangeNotification& notification) {
  switch (notification.action) {
    case FILE_ACTION_ADDED:
      LOGD(L"Added: {}{}", entry.path, notification.filename.first);
      break;
    case FILE_ACTION_REMOVED:
      LOGD(L"Removed: {}{}", entry.path, notification.filename.first);
      break;
    case FILE_ACTION_RENAMED_NEW_NAME:
      LOGD(L"Renamed (old): {0}{1}\nRenamed (new): {0}{2}", entry.path,
           notification.filename.second, notification.filename.first);
      break;
  }
}

void DirectoryMonitor::Callback(DirectoryChangeEntry& entry) {
  win::Lock lock(critical_section_);

  DirectoryChangeNotification* old_name_notification = nullptr;

  for (auto& notification : entry.notifications) {
    switch (notification.action) {
      case FILE_ACTION_RENAMED_OLD_NAME:
        old_name_notification = &notification;
        continue;
      case FILE_ACTION_RENAMED_NEW_NAME:
        if (old_name_notification) {
          notification.filename.second = old_name_notification->filename.first;
          old_name_notification = nullptr;
        }
        break;
    }

    if (notification.action != FILE_ACTION_REMOVED) {
      std::wstring path = entry.path + notification.filename.first;
      notification.type = FolderExists(path)
                              ? DirectoryChangeNotification::Type::Directory
                              : DirectoryChangeNotification::Type::File;
    } else {
      std::wstring extension = GetFileExtension(notification.filename.first);
      notification.type = !ValidateFileExtension(extension, 4)
                              ? DirectoryChangeNotification::Type::Directory
                              : DirectoryChangeNotification::Type::File;
    }

    LogFileAction(entry, notification);
    HandleChangeNotification(notification);
  }

  entry.notifications.clear();
}
