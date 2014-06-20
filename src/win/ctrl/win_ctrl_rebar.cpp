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

Rebar::Rebar(HWND hwnd) {
  SetWindowHandle(hwnd);
}

void Rebar::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = 0;
  cs.lpszClass = REBARCLASSNAME;
  cs.style = WS_CHILD | WS_VISIBLE | WS_TABSTOP |
             CCS_NODIVIDER | RBS_BANDBORDERS | RBS_VARHEIGHT;
}

void Rebar::OnCreate(HWND hwnd, LPCREATESTRUCT create_struct) {
  Window::OnCreate(hwnd, create_struct);
}

////////////////////////////////////////////////////////////////////////////////

UINT Rebar::GetBarHeight() {
  return SendMessage(RB_GETBARHEIGHT, 0, 0);
}

BOOL Rebar::InsertBand(LPREBARBANDINFO bar_info) {
  return SendMessage(RB_INSERTBAND, -1, reinterpret_cast<LPARAM>(bar_info));
}

BOOL Rebar::InsertBand(HWND hwnd_child,
                       UINT cx, UINT cx_header, UINT cx_ideal,
                       UINT cx_min_child,
                       UINT cy_child, UINT cy_integral,
                       UINT cy_max_child, UINT cy_min_child,
                       UINT mask, UINT style) {
  REBARBANDINFO rbi = {0};
  rbi.cbSize = REBARBANDINFO_V6_SIZE;
  rbi.cx = cx;
  rbi.cxHeader = cx_header;
  rbi.cxIdeal = cx_ideal;
  rbi.cxMinChild = cx_min_child;
  rbi.cyChild = cy_child;
  rbi.cyIntegral = cy_integral;
  rbi.cyMaxChild = cy_max_child;
  rbi.cyMinChild = cy_min_child;
  rbi.fMask = mask;
  rbi.fStyle = style;
  rbi.hwndChild = hwnd_child;

  return SendMessage(RB_INSERTBAND, -1, reinterpret_cast<LPARAM>(&rbi));
}

}  // namespace win