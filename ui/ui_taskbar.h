/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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

#ifndef UI_TASKBAR_H
#define UI_TASKBAR_H

#include <shobjidl.h>

// =============================================================================

/* Taskbar */

class CTaskbar {
public:
  CTaskbar();
  virtual ~CTaskbar();

  BOOL Create(HWND hwnd, HICON hIcon, LPCWSTR lpTooltip);
  BOOL Destroy();
  BOOL Modify(LPCWSTR lpTip);
  BOOL Tip(LPCWSTR lpText, LPCWSTR lpTitle, int iIconIndex);

private:
  HWND           m_hApp;
  NOTIFYICONDATA m_NID;
};

/* Taskbar list */

class CTaskbarList {
public:
  CTaskbarList();
  virtual ~CTaskbarList();

  void Initialize(HWND hwnd);
  void Release();
  void SetProgressState(TBPFLAG flag);
  void SetProgressValue(ULONGLONG ullValue, ULONGLONG ullTotal);

private:
  HWND           m_hWnd;
  ITaskbarList3* m_pTaskbarList;
};

// =============================================================================

extern CTaskbar Taskbar;
extern CTaskbarList TaskbarList;

extern const DWORD WM_TASKBARCALLBACK;
extern const DWORD WM_TASKBARCREATED;
extern const DWORD WM_TASKBARBUTTONCREATED;

#endif // UI_TASKBAR_H