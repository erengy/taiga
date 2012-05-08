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

#include "win_control.h"

namespace win32 {

// =============================================================================

void Rebar::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = 0;
  cs.lpszClass = REBARCLASSNAME;
  cs.style     = WS_CHILD | WS_VISIBLE | WS_TABSTOP | CCS_NODIVIDER | RBS_BANDBORDERS | RBS_VARHEIGHT;
}

void Rebar::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
  Window::OnCreate(hwnd, lpCreateStruct);
}

// =============================================================================

UINT Rebar::GetBarHeight() {
  return ::SendMessage(m_hWindow, RB_GETBARHEIGHT, 0, 0);
}

BOOL Rebar::InsertBand(LPREBARBANDINFO lpBarInfo) {
  return ::SendMessage(m_hWindow, RB_INSERTBAND, -1, reinterpret_cast<LPARAM>(lpBarInfo));
}

BOOL Rebar::InsertBand(HWND hwndChild, UINT cx, UINT cxHeader, UINT cxIdeal, UINT cxMinChild, 
                       UINT cyChild, UINT cyIntegral, UINT cyMaxChild, UINT cyMinChild, 
                       UINT fMask, UINT fStyle) {
  REBARBANDINFO rbi = {0};
  rbi.cbSize     = REBARBANDINFO_V6_SIZE;
  rbi.cx         = cx;
  rbi.cxHeader   = cxHeader;
  rbi.cxIdeal    = cxIdeal;
  rbi.cxMinChild = cxMinChild;
  rbi.cyChild    = cyChild;
  rbi.cyIntegral = cyIntegral;
  rbi.cyMaxChild = cyMaxChild;
  rbi.cyMinChild = cyMinChild;
  rbi.fMask      = fMask;
  rbi.fStyle     = fStyle;
  rbi.hwndChild  = hwndChild;

  return ::SendMessage(m_hWindow, RB_INSERTBAND, -1, reinterpret_cast<LPARAM>(&rbi));
}

} // namespace win32