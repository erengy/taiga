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

#ifndef TAIGA_WIN_TASKBAR_H
#define TAIGA_WIN_TASKBAR_H

#include <shobjidl.h>

namespace win {

class Taskbar {
public:
  Taskbar();
  ~Taskbar();

  BOOL Create(HWND hwnd, HICON icon, LPCWSTR tooltip);
  BOOL Destroy();
  BOOL Modify(LPCWSTR tip);
  BOOL Tip(LPCWSTR text, LPCWSTR title, int icon_index);

private:
  HWND hwnd_;
  NOTIFYICONDATA data_;
};

////////////////////////////////////////////////////////////////////////////////

class TaskbarList {
public:
  TaskbarList();
  ~TaskbarList();

  void Initialize(HWND hwnd);
  void Release();
  void SetProgressState(TBPFLAG flag);
  void SetProgressValue(ULONGLONG value, ULONGLONG total);

private:
  HWND hwnd_;
  ITaskbarList3* taskbar_list_;
};

}  // namespace win

////////////////////////////////////////////////////////////////////////////////

extern class win::Taskbar Taskbar;
extern class win::TaskbarList TaskbarList;

extern const DWORD WM_TASKBARCALLBACK;
extern const DWORD WM_TASKBARCREATED;
extern const DWORD WM_TASKBARBUTTONCREATED;

#endif  // TAIGA_WIN_TASKBAR_H