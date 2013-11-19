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

void Edit::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = WS_EX_CLIENTEDGE;
  cs.lpszClass = L"EDIT";
  cs.style     = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL;
}

void Edit::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
  Window::OnCreate(hwnd, lpCreateStruct);
}

// =============================================================================

void Edit::GetRect(LPRECT lprc) {
  ::SendMessage(m_hWindow, EM_GETRECT, 0, reinterpret_cast<LPARAM>(lprc));
}

void Edit::LimitText(int cchMax) {
  ::SendMessage(m_hWindow, EM_SETLIMITTEXT, cchMax, 0);
}

BOOL Edit::SetCueBannerText(LPCWSTR lpcwText, BOOL fDrawFocused) {
  return ::SendMessage(m_hWindow, EM_SETCUEBANNER, fDrawFocused, reinterpret_cast<LPARAM>(lpcwText));
}

void Edit::SetMargins(int iLeft, int iRight) {
  DWORD flags = 0;
  if (iLeft > -1)  flags |= EC_LEFTMARGIN;
  if (iRight > -1) flags |= EC_RIGHTMARGIN;
  ::SendMessage(m_hWindow, EM_SETMARGINS, flags, MAKELPARAM(iLeft, iRight));
}

void Edit::SetMultiLine(BOOL bEnabled) {
  // TODO: We have to re-create the control, this does not work.
  if (bEnabled) {
    SetStyle(ES_MULTILINE | ES_AUTOVSCROLL, ES_AUTOHSCROLL);
  } else {
    SetStyle(ES_AUTOHSCROLL, ES_MULTILINE | ES_AUTOVSCROLL);
  }
}

void Edit::SetPasswordChar(UINT ch) {
  ::SendMessage(m_hWindow, EM_SETPASSWORDCHAR, ch, 0);
}

void Edit::SetReadOnly(BOOL bReadOnly) {
  ::SendMessage(m_hWindow, EM_SETREADONLY, bReadOnly, 0);
}

void Edit::SetRect(LPRECT lprc) {
  ::SendMessage(m_hWindow, EM_SETRECT, 0, reinterpret_cast<LPARAM>(lprc));
}

void Edit::SetSel(int ichStart, int ichEnd) {
  ::SendMessage(m_hWindow, EM_SETSEL, ichStart, ichEnd);
  ::SendMessage(m_hWindow, EM_SCROLLCARET, 0, 0);
}

BOOL Edit::ShowBalloonTip(LPCWSTR pszText, LPCWSTR pszTitle, INT ttiIcon) {
  EDITBALLOONTIP ebt;
  ebt.cbStruct = sizeof(EDITBALLOONTIP);
  ebt.pszText  = pszText;
  ebt.pszTitle = pszTitle;
  ebt.ttiIcon  = ttiIcon;
  return ::SendMessage(m_hWindow, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));
}

}  // namespace win