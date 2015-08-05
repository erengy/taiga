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

#include "win_ctrl.h"

namespace win {

Tooltip::Tooltip(HWND hwnd) {
  SetWindowHandle(hwnd);
}

void Tooltip::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = 0;
  cs.lpszClass = TOOLTIPS_CLASS;
  cs.style = WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP;
}

void Tooltip::OnCreate(HWND hwnd, LPCREATESTRUCT create_struct) {
  ::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  Window::OnCreate(hwnd, create_struct);
}

////////////////////////////////////////////////////////////////////////////////

BOOL Tooltip::AddTip(UINT id, LPCWSTR text, LPCWSTR title, LPRECT rect,
                     bool window_id) {
  TOOLINFO ti = {0};
  ti.cbSize = sizeof(TOOLINFO);
  ti.hwnd = parent_;
  ti.hinst = instance_;
  ti.lpszText = const_cast<LPWSTR>(text);
  ti.uFlags = TTF_SUBCLASS | (window_id ? TTF_IDISHWND : 0);
  ti.uId = static_cast<UINT_PTR>(id);

  if (rect)
    ti.rect = *rect;

  BOOL result = SendMessage(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
  if (result && title)
    result = SendMessage(TTM_SETTITLE, TTI_INFO,
                         reinterpret_cast<LPARAM>(title));

  return result;
}

void Tooltip::DeleteTip(UINT id) {
  TOOLINFO ti = {0};
  ti.cbSize = sizeof(TOOLINFO);
  ti.hwnd = parent_;
  ti.uId = static_cast<UINT_PTR>(id);

  SendMessage(TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&ti));
}

void Tooltip::NewToolRect(UINT id, LPRECT rect) {
  TOOLINFO ti = {0};
  ti.cbSize = sizeof(TOOLINFO);
  ti.hwnd = parent_;
  ti.uId = static_cast<UINT_PTR>(id);

  if (rect)
    ti.rect = *rect;

  SendMessage(TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&ti));
}

void Tooltip::SetDelayTime(long autopop, long initial, long reshow) {
  SendMessage(TTM_SETDELAYTIME, TTDT_AUTOPOP, autopop);
  SendMessage(TTM_SETDELAYTIME, TTDT_INITIAL, initial);
  SendMessage(TTM_SETDELAYTIME, TTDT_RESHOW,  reshow);
}

void Tooltip::SetMaxWidth(long width) {
  SendMessage(TTM_SETMAXTIPWIDTH, 0, width);
}

void Tooltip::UpdateText(UINT id, LPCWSTR text) {
  TOOLINFO ti = {0};
  ti.cbSize = sizeof(TOOLINFO);
  ti.hinst = instance_;
  ti.hwnd = parent_;
  ti.lpszText = const_cast<LPWSTR>(text);
  ti.uId = static_cast<UINT_PTR>(id);

  SendMessage(TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&ti));
}

void Tooltip::UpdateTitle(LPCWSTR title) {
  SendMessage(TTM_SETTITLE, title ? 0 : 1, reinterpret_cast<LPARAM>(title));
}

}  // namespace win