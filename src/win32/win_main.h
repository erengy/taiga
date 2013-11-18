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

#ifndef WIN_MAIN_H
#define WIN_MAIN_H

#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>

#include <map>
#include <string>
#include <vector>

using std::string;
using std::vector;
using std::wstring;

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

namespace win32 {

class Window;

// =============================================================================

/* Window map */

typedef std::map<HWND, Window*> WndMap;

class WindowMap {
public:
  void Add(HWND hWnd, Window* pWindow);
  void Clear();
  Window* GetWindow(HWND hWnd);
  void Remove(HWND hWnd);
  void Remove(Window* pWindow);

private:
  WndMap m_WindowMap;
};

extern class WindowMap WindowMap;

// =============================================================================

/* Application */

class App {
public:
  App();
  virtual ~App();

  virtual BOOL InitInstance();
  virtual int MessageLoop();
  virtual void PostQuitMessage(int nExitCode = 0);
  virtual int Run();

  int GetVersionMajor() { return m_VersionMajor; }
  int GetVersionMinor() { return m_VersionMinor; }
  int GetVersionRevision() { return m_VersionRevision; }
  void SetVersionInfo(int major, int minor, int revision);

  wstring GetCurrentDirectory();
  HINSTANCE GetInstanceHandle() const;
  wstring GetModulePath();
  BOOL InitCommonControls(DWORD dwFlags);
  BOOL SetCurrentDirectory(const wstring& directory);

private:
  HINSTANCE m_hInstance;
  int m_VersionMajor, m_VersionMinor, m_VersionRevision;
};

// =============================================================================

enum WinVersion {
  VERSION_PRE_XP = 0,
  VERSION_XP,
  VERSION_SERVER_2003,
  VERSION_VISTA,
  VERSION_SERVER_2008,
  VERSION_WIN7,
  VERSION_WIN8,
  VERSION_WIN8_1,
  VERSION_UNKNOWN
};

WinVersion GetWinVersion();

} // namespace win32

#endif // WIN_MAIN_H