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

#include "win_main.h"

CWindowMap WindowMap;

// =============================================================================

void CWindowMap::Add(HWND hWnd, CWindow* pWindow) {
  if (hWnd != NULL) {
    if (GetWindow(hWnd) == NULL) {
      m_WindowMap.insert(std::make_pair(hWnd, pWindow));
    }
  }
}

void CWindowMap::Clear() {
  if (m_WindowMap.empty()) return;
  for (WndMap::iterator i = m_WindowMap.begin(); i != m_WindowMap.end(); ++i) {
    HWND hWnd = i->first;
    if (::IsWindow(hWnd)) ::DestroyWindow(hWnd);
  }
  m_WindowMap.clear();
}

CWindow* CWindowMap::GetWindow(HWND hWnd) {
  if (m_WindowMap.empty()) return NULL;
  WndMap::iterator i = m_WindowMap.find(hWnd);
  if (i != m_WindowMap.end()) {
    return i->second;
  } else {
    return NULL;
  }
}

void CWindowMap::Remove(HWND hWnd) {
  if (m_WindowMap.empty()) return;
  for (WndMap::iterator i = m_WindowMap.begin(); i != m_WindowMap.end(); ++i) {
    if (hWnd == i->first) {
      m_WindowMap.erase(i);
      return;
    }
  }
}

void CWindowMap::Remove(CWindow* pWindow) {
  if (m_WindowMap.empty()) return;
  for (WndMap::iterator i = m_WindowMap.begin(); i != m_WindowMap.end(); ++i) {
    if (pWindow == i->second) {
      m_WindowMap.erase(i);
      return;
    }
  }
}

// =============================================================================

CApp::CApp() {
  m_hInstance = ::GetModuleHandle(NULL);
}

CApp::~CApp() {
  WindowMap.Clear();
}

BOOL CApp::InitInstance() {
  return TRUE;
}

int CApp::MessageLoop() {
  MSG msg;
  while (::GetMessage(&msg, NULL, 0, 0)) {
    BOOL processed = FALSE;
    if ((msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST) || 
      (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)) {
        for (HWND hwnd = msg.hwnd; hwnd != NULL; hwnd = ::GetParent(hwnd)) {
          CWindow* pWindow = WindowMap.GetWindow(hwnd);
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

void CApp::PostQuitMessage(int nExitCode) {
  ::PostQuitMessage(nExitCode);
}

int CApp::Run() {
  if (InitInstance()) {
    return MessageLoop();
  } else {
    ::PostQuitMessage(-1);
    return -1;
  }
}

wstring CApp::GetCurrentDirectory() {
  WCHAR buff[MAX_PATH];
  DWORD len = ::GetCurrentDirectory(MAX_PATH, buff);
  if (len > 0 && len < MAX_PATH) {
    return buff;
  } else {
    return L"";
  }
}

HINSTANCE CApp::GetInstanceHandle() const {
  return m_hInstance;
}

wstring CApp::GetModulePath() {
  WCHAR buff[MAX_PATH];
  ::GetModuleFileName(m_hInstance, buff, MAX_PATH);
  return buff;
}

BOOL CApp::InitCommonControls(DWORD dwFlags) {
  INITCOMMONCONTROLSEX icc;
  icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icc.dwICC = dwFlags;
  return ::InitCommonControlsEx(&icc);
}

BOOL CApp::SetCurrentDirectory(const wstring& directory) {
  return ::SetCurrentDirectory(directory.c_str());
}

// =============================================================================

WinVersion GetWinVersion() {
  static bool checked_version = false;
  static WinVersion win_version = WINVERSION_PRE_2000;
  if (!checked_version) {
    OSVERSIONINFOEX version_info;
    version_info.dwOSVersionInfoSize = sizeof(version_info);
    GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&version_info));
    if (version_info.dwMajorVersion == 5) {
      switch (version_info.dwMinorVersion) {
        case 0:
          win_version = WINVERSION_2000;
          break;
        case 1:
          win_version = WINVERSION_XP;
          break;
        case 2:
        default:
          win_version = WINVERSION_SERVER_2003;
          break;
        }
    } else if (version_info.dwMajorVersion == 6) {
      if (version_info.wProductType != VER_NT_WORKSTATION) {
        win_version = WINVERSION_2008;
      } else {
        if (version_info.dwMinorVersion == 0) {
          win_version = WINVERSION_VISTA;
        } else {
          win_version = WINVERSION_WIN7;
        }
      }
    } else if (version_info.dwMajorVersion > 6) {
      win_version = WINVERSION_WIN7;
    }
    checked_version = true;
  }
  return win_version;
}

void DEBUG_PRINT(wstring text) {
  #ifdef _DEBUG
  ::OutputDebugString(text.c_str());
  #else
  UNREFERENCED_PARAMETER(text);
  #endif
}