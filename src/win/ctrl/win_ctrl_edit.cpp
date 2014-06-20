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

Edit::Edit(HWND hwnd) {
  SetWindowHandle(hwnd);
}

void Edit::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = WS_EX_CLIENTEDGE;
  cs.lpszClass = L"EDIT";
  cs.style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL;
}

void Edit::OnCreate(HWND hwnd, LPCREATESTRUCT create_struct) {
  Window::OnCreate(hwnd, create_struct);
}

////////////////////////////////////////////////////////////////////////////////

void Edit::GetRect(LPRECT rect) {
  SendMessage(EM_GETRECT, 0, reinterpret_cast<LPARAM>(rect));
}

void Edit::LimitText(int limit) {
  SendMessage(EM_SETLIMITTEXT, limit, 0);
}

BOOL Edit::SetCueBannerText(LPCWSTR text, BOOL draw_focused) {
  return SendMessage(EM_SETCUEBANNER, draw_focused,
                     reinterpret_cast<LPARAM>(text));
}

void Edit::SetMargins(int left, int right) {
  DWORD flags = 0;

  if (left > -1)
    flags |= EC_LEFTMARGIN;
  if (right > -1)
    flags |= EC_RIGHTMARGIN;

  SendMessage(EM_SETMARGINS, flags, MAKELPARAM(left, right));
}

void Edit::SetMultiLine(BOOL enabled) {
  // TODO: We have to re-create the control, this does not work.
  if (enabled) {
    SetStyle(ES_MULTILINE | ES_AUTOVSCROLL, ES_AUTOHSCROLL);
  } else {
    SetStyle(ES_AUTOHSCROLL, ES_MULTILINE | ES_AUTOVSCROLL);
  }
}

void Edit::SetPasswordChar(UINT ch) {
  SendMessage(EM_SETPASSWORDCHAR, ch, 0);
}

void Edit::SetReadOnly(BOOL read_only) {
  SendMessage(EM_SETREADONLY, read_only, 0);
}

void Edit::SetRect(LPRECT rect) {
  SendMessage(EM_SETRECT, 0, reinterpret_cast<LPARAM>(rect));
}

void Edit::SetSel(int start, int end) {
  SendMessage(EM_SETSEL, start, end);
  SendMessage(EM_SCROLLCARET, 0, 0);
}

BOOL Edit::ShowBalloonTip(LPCWSTR text, LPCWSTR title, INT icon) {
  EDITBALLOONTIP ebt;
  ebt.cbStruct = sizeof(EDITBALLOONTIP);
  ebt.pszText = text;
  ebt.pszTitle = title;
  ebt.ttiIcon = icon;

  return SendMessage(EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));
}

}  // namespace win