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

#ifndef ACCESSIBILITY_H
#define ACCESSIBILITY_H

#include "std.h"
#include <OleAcc.h>

// =============================================================================

class AccessibleChild {
public:
  wstring name, role, value;

public:
  vector<AccessibleChild> children;
};

class AccessibleObject {
public:
  AccessibleObject();
  virtual ~AccessibleObject();

  HRESULT FromWindow(HWND hwnd, DWORD object_id = OBJID_CLIENT);
  
  HRESULT BuildChildren(vector<AccessibleChild>& children, IAccessible* acc = nullptr, LPARAM param = 0L);
  HRESULT GetChildCount(long* child_count, IAccessible* acc = nullptr);
  virtual bool AllowChildTraverse(AccessibleChild& child, LPARAM param = 0L);

  HRESULT GetName(wstring& name, long child_id = CHILDID_SELF, IAccessible* acc = nullptr);
  HRESULT GetRole(wstring& role, long child_id = CHILDID_SELF, IAccessible* acc = nullptr);
  HRESULT GetValue(wstring& value, long child_id = CHILDID_SELF, IAccessible* acc = nullptr);

  HWINEVENTHOOK Hook(DWORD eventMin, DWORD eventMax, 
    HMODULE hmodWinEventProc, WINEVENTPROC pfnWinEventProc,
    DWORD idProcess, DWORD idThread, DWORD dwFlags);
  bool IsHooked();
  void Unhook();

  void Release();

public:
  vector<AccessibleChild> children;

private:
  IAccessible* acc_;
  HWINEVENTHOOK win_event_hook_;
};

#endif // ACCESSIBILITY_H