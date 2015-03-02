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

#ifndef TAIGA_WIN_ACCESSIBILITY_H
#define TAIGA_WIN_ACCESSIBILITY_H

#include <string>
#include <OleAcc.h>
#include <vector>

namespace win {

struct AccessibleChild {
  AccessibleChild();

  std::wstring name;
  DWORD role;
  std::wstring value;
  std::vector<AccessibleChild> children;
};

class AccessibleObject {
public:
  AccessibleObject();
  virtual ~AccessibleObject();

  HRESULT FromWindow(HWND hwnd, DWORD object_id = OBJID_CLIENT);
  void Release();

  HRESULT GetName(std::wstring& name, long child_id = CHILDID_SELF,
                  IAccessible* acc = nullptr);
  HRESULT GetRole(DWORD& role, long child_id = CHILDID_SELF,
                  IAccessible* acc = nullptr);
  HRESULT GetRole(std::wstring& role, long child_id = CHILDID_SELF,
                  IAccessible* acc = nullptr);
  HRESULT GetValue(std::wstring& value, long child_id = CHILDID_SELF,
                   IAccessible* acc = nullptr);

  HRESULT BuildChildren(std::vector<AccessibleChild>& children,
                        IAccessible* acc = nullptr, LPARAM param = 0L);
  HRESULT GetChildCount(long* child_count, IAccessible* acc = nullptr);

  virtual bool AllowChildTraverse(AccessibleChild& child, LPARAM param = 0L);

#ifdef _DEBUG
  HWINEVENTHOOK Hook(DWORD eventMin, DWORD eventMax,
                     HMODULE hmodWinEventProc, WINEVENTPROC pfnWinEventProc,
                     DWORD idProcess, DWORD idThread, DWORD dwFlags);
  bool IsHooked();
  void Unhook();
#endif

  std::vector<AccessibleChild> children;

private:
  IAccessible* acc_;
  HWINEVENTHOOK win_event_hook_;
};

}  // namespace win

#endif  // TAIGA_WIN_ACCESSIBILITY_H