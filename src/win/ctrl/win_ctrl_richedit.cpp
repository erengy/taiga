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

RichEdit::RichEdit() {
  m_hInstRichEdit = ::LoadLibrary(L"riched20.dll");
}

RichEdit::RichEdit(HWND hWnd) {
  m_hInstRichEdit = ::LoadLibrary(L"riched20.dll");
  SetWindowHandle(hWnd);
}

RichEdit::~RichEdit() {
  if (m_hInstRichEdit) {
    ::FreeLibrary(m_hInstRichEdit);
  }
}

void RichEdit::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = WS_EX_STATICEDGE;
  cs.lpszClass = RICHEDIT_CLASS;
  cs.style     = WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP | WS_VISIBLE | WS_VSCROLL | ES_AUTOHSCROLL;
}

void RichEdit::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
  Window::OnCreate(hwnd, lpCreateStruct);
}

// =============================================================================

void RichEdit::GetSel(CHARRANGE* cr) {
  ::SendMessage(window_, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(cr));
}

wstring RichEdit::GetTextRange(CHARRANGE* cr) {
  wstring str(1024, '\0');

  TEXTRANGE tr;
  tr.chrg.cpMax = cr->cpMax;
  tr.chrg.cpMin = cr->cpMin;
  tr.lpstrText = (LPWSTR)str.data();

  ::SendMessage(window_, EM_GETTEXTRANGE , 0, reinterpret_cast<LPARAM>(&tr));
  
  return str;
}

void RichEdit::HideSelection(BOOL bHide) {
  ::SendMessage(window_, EM_HIDESELECTION, bHide, 0);
}

BOOL RichEdit::SetCharFormat(DWORD dwFormat, CHARFORMAT* cf) {
  return ::SendMessage(window_, EM_SETCHARFORMAT, dwFormat, reinterpret_cast<LPARAM>(cf));
}

DWORD RichEdit::SetEventMask(DWORD dwFlags) {
  return ::SendMessage(window_, EM_SETEVENTMASK, 0, dwFlags);
}

void RichEdit::SetSel(int ichStart, int ichEnd) {
  ::SendMessage(window_, EM_SETSEL, ichStart, ichEnd);
}

void RichEdit::SetSel(CHARRANGE* cr) {
  ::SendMessage(window_, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(cr));
}

UINT RichEdit::SetTextEx(const string& str) {
  SETTEXTEX ste;
  return ::SendMessage(window_, EM_SETTEXTEX, 
                       reinterpret_cast<WPARAM>(&ste), 
                       reinterpret_cast<LPARAM>(str.data()));
}

}  // namespace win