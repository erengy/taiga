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

RichEdit::RichEdit() {
  module_msftedit_ = ::LoadLibrary(L"msftedit.dll");
  module_riched20_ = ::LoadLibrary(L"riched20.dll");
}

RichEdit::RichEdit(HWND hwnd) {
  module_msftedit_ = ::LoadLibrary(L"msftedit.dll");
  module_riched20_ = ::LoadLibrary(L"riched20.dll");
  SetWindowHandle(hwnd);
}

RichEdit::~RichEdit() {
  if (module_msftedit_)
    ::FreeLibrary(module_msftedit_);
  if (module_riched20_)
    ::FreeLibrary(module_riched20_);
}

void RichEdit::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = WS_EX_STATICEDGE;
  cs.lpszClass = RICHEDIT_CLASS;
  cs.style = WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP | WS_VISIBLE | WS_VSCROLL |
             ES_AUTOHSCROLL;
}

void RichEdit::OnCreate(HWND hwnd, LPCREATESTRUCT create_struct) {
  Window::OnCreate(hwnd, create_struct);
}

////////////////////////////////////////////////////////////////////////////////

void RichEdit::GetSel(CHARRANGE* cr) {
  SendMessage(EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(cr));
}

std::wstring RichEdit::GetTextRange(CHARRANGE* cr) {
  std::wstring text(1024, L'\0');

  TEXTRANGE tr;
  tr.chrg.cpMax = cr->cpMax;
  tr.chrg.cpMin = cr->cpMin;
  tr.lpstrText = const_cast<LPWSTR>(text.data());

  SendMessage(EM_GETTEXTRANGE , 0, reinterpret_cast<LPARAM>(&tr));

  return text;
}

void RichEdit::HideSelection(BOOL hide) {
  SendMessage(EM_HIDESELECTION, hide, 0);
}

BOOL RichEdit::SetCharFormat(DWORD format, CHARFORMAT* cf) {
  return SendMessage(EM_SETCHARFORMAT, format, reinterpret_cast<LPARAM>(cf));
}

DWORD RichEdit::SetEventMask(DWORD flags) {
  return SendMessage(EM_SETEVENTMASK, 0, flags);
}

void RichEdit::SetSel(int start, int end) {
  SendMessage(EM_SETSEL, start, end);
}

void RichEdit::SetSel(CHARRANGE* cr) {
  SendMessage(EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(cr));
}

UINT RichEdit::SetTextEx(const std::string& text) {
  SETTEXTEX ste;
  ste.codepage = CP_UTF8;
  ste.flags = ST_DEFAULT;

  return SendMessage(EM_SETTEXTEX,
                     reinterpret_cast<WPARAM>(&ste),
                     reinterpret_cast<LPARAM>(text.data()));
}

}  // namespace win