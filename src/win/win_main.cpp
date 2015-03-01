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

#include "win_main.h"
#include "win_window.h"

namespace win {

class WindowMap WindowMap;

void WindowMap::Add(HWND hwnd, Window* window) {
  if (hwnd != nullptr) {
    if (!GetWindow(hwnd)) {
      window_map_.insert(std::make_pair(hwnd, window));
    }
  }
}

void WindowMap::Clear() {
  if (window_map_.empty())
    return;

  for (auto it = window_map_.begin(); it != window_map_.end(); ++it) {
    HWND hwnd = it->first;
    if (::IsWindow(hwnd))
      ::DestroyWindow(hwnd);
  }

  window_map_.clear();
}

Window* WindowMap::GetWindow(HWND hwnd) {
  if (window_map_.empty())
    return nullptr;

  auto it = window_map_.find(hwnd);
  if (it != window_map_.end())
    return it->second;

  return nullptr;
}

void WindowMap::Remove(HWND hwnd) {
  if (window_map_.empty())
    return;

  for (auto it = window_map_.begin(); it != window_map_.end(); ++it) {
    if (hwnd == it->first) {
      window_map_.erase(it);
      return;
    }
  }
}

void WindowMap::Remove(Window* window) {
  if (window_map_.empty())
    return;

  for (auto it = window_map_.begin(); it != window_map_.end(); ++it) {
    if (window == it->second) {
      window_map_.erase(it);
      return;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

App::App() {
  instance_ = ::GetModuleHandle(nullptr);
}

App::~App() {
  WindowMap.Clear();
}

BOOL App::InitCommonControls(DWORD flags) const {
  INITCOMMONCONTROLSEX icc;
  icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icc.dwICC = flags;

  return ::InitCommonControlsEx(&icc);
}

BOOL App::InitInstance() {
  return TRUE;
}

int App::MessageLoop() {
  MSG msg;

  while (::GetMessage(&msg, nullptr, 0, 0)) {
    BOOL processed = FALSE;
    if ((msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST) ||
        (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)) {
      for (HWND hwnd = msg.hwnd; hwnd != nullptr; hwnd = ::GetParent(hwnd)) {
        auto window = WindowMap.GetWindow(hwnd);
        if (window) {
          processed = window->PreTranslateMessage(&msg);
          if (processed)
            break;
        }
      }
    }

    if (!processed) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }
  }

  return static_cast<int>(LOWORD(msg.wParam));
}

void App::PostQuitMessage(int exit_code) {
  ::PostQuitMessage(exit_code);
}

int App::Run() {
  if (InitInstance()) {
    return MessageLoop();
  } else {
    ::PostQuitMessage(-1);
    return -1;
  }
}

std::wstring App::GetCurrentDirectory() const {
  WCHAR path[MAX_PATH];
  DWORD len = ::GetCurrentDirectory(MAX_PATH, path);
  if (len > 0 && len < MAX_PATH)
    return path;

  return std::wstring();
}

HINSTANCE App::GetInstanceHandle() const {
  return instance_;
}

std::wstring App::GetModulePath() const {
  WCHAR path[MAX_PATH];
  ::GetModuleFileName(instance_, path, MAX_PATH);
  return path;
}

BOOL App::SetCurrentDirectory(const std::wstring& directory) {
  return ::SetCurrentDirectory(directory.c_str());
}

////////////////////////////////////////////////////////////////////////////////

Version GetVersion() {
  static bool checked = false;
  static Version version = kVersionPreXp;

  if (!checked) {
    OSVERSIONINFOEX version_info;
    version_info.dwOSVersionInfoSize = sizeof(version_info);
    GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&version_info));

    if (version_info.dwMajorVersion == 5) {
      switch (version_info.dwMinorVersion) {
        case 0:
          version = kVersionPreXp;
          break;
        case 1:
          version = kVersionXp;
          break;
        case 2:
        default:
          version = kVersionServer2003;
          break;
      }
    } else if (version_info.dwMajorVersion == 6) {
      if (version_info.wProductType != VER_NT_WORKSTATION) {
        version = kVersionServer2008;
      } else {
        switch (version_info.dwMinorVersion) {
          case 0:
            version = kVersionVista;
            break;
          case 1:
            version = kVersion7;
            break;
          case 2:
            version = kVersion8;
            break;
          case 3:
          default:
            version = kVersion8_1;
            break;
        }
      }
    } else if (version_info.dwMajorVersion == 10) {
      switch (version_info.dwMinorVersion) {
        case 0:
        default:
          version = kVersion10;
          break;
      }
    } else if (version_info.dwMajorVersion > 10) {
      version = kVersionUnknown;
    }

    checked = true;
  }

  return version;
}

}  // namespace win