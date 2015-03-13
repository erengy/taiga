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

#ifndef TAIGA_WIN_MAIN_H
#define TAIGA_WIN_MAIN_H

#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>

#include <map>
#include <string>
#include <vector>

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

namespace win {

class Window;

class WindowMap {
public:
  void Add(HWND hwnd, Window* window);
  void Clear();
  Window* GetWindow(HWND hwnd);
  void Remove(HWND hwnd);
  void Remove(Window* window);

private:
  std::map<HWND, Window*> window_map_;
};

extern class WindowMap WindowMap;

////////////////////////////////////////////////////////////////////////////////

class App {
public:
  App();
  virtual ~App();

  BOOL InitCommonControls(DWORD flags) const;
  virtual BOOL InitInstance();
  virtual int MessageLoop();
  virtual void PostQuitMessage(int exit_code = 0);
  virtual int Run();

  std::wstring GetCurrentDirectory() const;
  HINSTANCE GetInstanceHandle() const;
  std::wstring GetModulePath() const;
  BOOL SetCurrentDirectory(const std::wstring& directory);

private:
  HINSTANCE instance_;
};

////////////////////////////////////////////////////////////////////////////////

enum Version {
  kVersionPreXp = 0,
  kVersionXp,
  kVersionServer2003,
  kVersionVista,
  kVersionServer2008,
  kVersion7,
  kVersion8,
  kVersion8_1,
  kVersion10,
  kVersionUnknown
};

Version GetVersion();

}  // namespace win

#endif  // TAIGA_WIN_MAIN_H