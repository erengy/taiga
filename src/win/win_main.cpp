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

#include "win_main.h"
#include "win_window.h"

namespace win {

class WindowMap WindowMap;

// =============================================================================

void WindowMap::Add(HWND hWnd, Window* pWindow) {
  if (hWnd != NULL) {
    if (GetWindow(hWnd) == NULL) {
      m_WindowMap.insert(std::make_pair(hWnd, pWindow));
    }
  }
}

void WindowMap::Clear() {
  if (m_WindowMap.empty()) return;
  for (WndMap::iterator i = m_WindowMap.begin(); i != m_WindowMap.end(); ++i) {
    HWND hWnd = i->first;
    if (::IsWindow(hWnd)) ::DestroyWindow(hWnd);
  }
  m_WindowMap.clear();
}

Window* WindowMap::GetWindow(HWND hWnd) {
  if (m_WindowMap.empty()) return NULL;
  WndMap::iterator i = m_WindowMap.find(hWnd);
  if (i != m_WindowMap.end()) {
    return i->second;
  } else {
    return NULL;
  }
}

void WindowMap::Remove(HWND hWnd) {
  if (m_WindowMap.empty()) return;
  for (WndMap::iterator i = m_WindowMap.begin(); i != m_WindowMap.end(); ++i) {
    if (hWnd == i->first) {
      m_WindowMap.erase(i);
      return;
    }
  }
}

void WindowMap::Remove(Window* pWindow) {
  if (m_WindowMap.empty()) return;
  for (WndMap::iterator i = m_WindowMap.begin(); i != m_WindowMap.end(); ++i) {
    if (pWindow == i->second) {
      m_WindowMap.erase(i);
      return;
    }
  }
}

// =============================================================================

App::App() : 
  m_VersionMajor(1), m_VersionMinor(0), m_VersionRevision(0)
{
  m_hInstance = ::GetModuleHandle(NULL);
}

App::~App() {
  WindowMap.Clear();
}

BOOL App::InitInstance() {
  return TRUE;
}

int App::MessageLoop() {
  MSG msg;
  while (::GetMessage(&msg, NULL, 0, 0)) {
    BOOL processed = FALSE;
    if ((msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST) || 
      (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)) {
        for (HWND hwnd = msg.hwnd; hwnd != NULL; hwnd = ::GetParent(hwnd)) {
          Window* pWindow = WindowMap.GetWindow(hwnd);
          if (pWindow) {
            processed = pWindow->PreTranslateMessage(&msg);
            if (processed) break;
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

void App::PostQuitMessage(int nExitCode) {
  ::PostQuitMessage(nExitCode);
}

int App::Run() {
  if (InitInstance()) {
    return MessageLoop();
  } else {
    ::PostQuitMessage(-1);
    return -1;
  }
}

void App::SetVersionInfo(int major, int minor, int revision) {
  m_VersionMajor = major;
  m_VersionMinor = minor;
  m_VersionRevision = revision;
}

wstring App::GetCurrentDirectory() {
  WCHAR buff[MAX_PATH];
  DWORD len = ::GetCurrentDirectory(MAX_PATH, buff);
  if (len > 0 && len < MAX_PATH) {
    return buff;
  } else {
    return L"";
  }
}

HINSTANCE App::GetInstanceHandle() const {
  return m_hInstance;
}

wstring App::GetModulePath() {
  WCHAR buff[MAX_PATH];
  ::GetModuleFileName(m_hInstance, buff, MAX_PATH);
  return buff;
}

BOOL App::InitCommonControls(DWORD dwFlags) {
  INITCOMMONCONTROLSEX icc;
  icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icc.dwICC = dwFlags;
  return ::InitCommonControlsEx(&icc);
}

BOOL App::SetCurrentDirectory(const wstring& directory) {
  return ::SetCurrentDirectory(directory.c_str());
}

// =============================================================================

WinVersion GetWinVersion() {
  static bool checked_version = false;
  static WinVersion win_version = VERSION_PRE_XP;
  if (!checked_version) {
    OSVERSIONINFOEX version_info;
    version_info.dwOSVersionInfoSize = sizeof(version_info);
    GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&version_info));
    if (version_info.dwMajorVersion == 5) {
      switch (version_info.dwMinorVersion) {
        case 0:
          win_version = VERSION_PRE_XP;
          break;
        case 1:
          win_version = VERSION_XP;
          break;
        case 2:
        default:
          win_version = VERSION_SERVER_2003;
          break;
        }
    } else if (version_info.dwMajorVersion == 6) {
      if (version_info.wProductType != VER_NT_WORKSTATION) {
        win_version = VERSION_SERVER_2008;
      } else {
        switch (version_info.dwMinorVersion) {
          case 0:
            win_version = VERSION_VISTA;
            break;
          case 1:
            win_version = VERSION_WIN7;
            break;
          case 2:
            win_version = VERSION_WIN8;
            break;
          default:
            win_version = VERSION_WIN8_1;
            break;
        }
      }
    } else if (version_info.dwMajorVersion > 6) {
      win_version = VERSION_UNKNOWN;
    }
    checked_version = true;
  }
  return win_version;
}

}  // namespace win