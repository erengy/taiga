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

#include "win_ctrl.h"

namespace win {

// =============================================================================

void Tooltip::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = NULL;
  cs.lpszClass = TOOLTIPS_CLASS;
  cs.style     = WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP;
}

void Tooltip::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
  ::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  Window::OnCreate(hwnd, lpCreateStruct);
}

// =============================================================================

BOOL Tooltip::AddTip(UINT uID, LPCWSTR lpText, LPCWSTR lpTitle, LPRECT rcArea, bool bWindowID) {
  TOOLINFO ti;
  ti.cbSize   = sizeof(TOOLINFO);
  ti.hwnd     = parent_;
  ti.hinst    = instance_;
  ti.lpszText = (LPWSTR)lpText;
  ti.uFlags   = TTF_SUBCLASS | (bWindowID ? TTF_IDISHWND : NULL);
  ti.uId      = (UINT_PTR)uID;
  if (rcArea) ti.rect = *rcArea;

  BOOL lResult = ::SendMessage(window_, TTM_ADDTOOL, 0, (LPARAM)&ti);
  if (lResult && lpTitle) {
    lResult = ::SendMessage(window_, TTM_SETTITLE, 1, (LPARAM)lpTitle);
  }
  return lResult;
}

BOOL Tooltip::DeleteTip(UINT uID) {
  TOOLINFO ti;
  ti.cbSize = sizeof(TOOLINFO);
  ti.hwnd   = parent_;
  ti.uId    = (UINT_PTR)uID;

  return ::SendMessage(window_, TTM_DELTOOL, 0, (LPARAM)&ti);
}

void Tooltip::SetDelayTime(long lAutopop, long lInitial, long lReshow) {
  ::SendMessage(window_, TTM_SETDELAYTIME, TTDT_AUTOPOP, lAutopop);
  ::SendMessage(window_, TTM_SETDELAYTIME, TTDT_INITIAL, lInitial);
  ::SendMessage(window_, TTM_SETDELAYTIME, TTDT_RESHOW,  lReshow);
}

void Tooltip::SetMaxWidth(long lWidth) {
  ::SendMessage(window_, TTM_SETMAXTIPWIDTH, NULL, lWidth);
}

void Tooltip::UpdateText(UINT uID, LPCWSTR lpText) {
  TOOLINFO ti;
  ti.cbSize   = sizeof(TOOLINFO);
  ti.hinst    = instance_;
  ti.hwnd     = parent_;
  ti.lpszText = (LPWSTR)lpText;
  ti.uId      = (UINT_PTR)uID;

  ::SendMessage(window_, TTM_UPDATETIPTEXT, NULL, (LPARAM)&ti);
}

void Tooltip::UpdateTitle(LPCWSTR lpTitle) {
  ::SendMessage(window_, TTM_SETTITLE, lpTitle ? 0 : 1, (LPARAM)lpTitle);
}

}  // namespace win