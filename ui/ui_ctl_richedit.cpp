/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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

#include "ui_control.h"

// =============================================================================

CRichEdit::CRichEdit() {
  m_hInstRichEdit = ::LoadLibrary(L"riched20.dll");
}

CRichEdit::~CRichEdit() {
  if (m_hInstRichEdit) {
    ::FreeLibrary(m_hInstRichEdit);
  }
}

void CRichEdit::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = WS_EX_STATICEDGE;
  cs.lpszClass = RICHEDIT_CLASS;
  cs.style     = WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP | WS_VISIBLE | WS_VSCROLL | ES_AUTOHSCROLL;
}

BOOL CRichEdit::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
  return TRUE;
}

// =============================================================================

void CRichEdit::GetSel(CHARRANGE* cr) {
  ::SendMessage(m_hWindow, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(cr));
}

void CRichEdit::HideSelection(BOOL bHide) {
  ::SendMessage(m_hWindow, EM_HIDESELECTION, bHide, 0);
}

BOOL CRichEdit::SetCharFormat(DWORD dwFormat, CHARFORMAT* cf) {
  return ::SendMessage(m_hWindow, EM_SETCHARFORMAT, dwFormat, reinterpret_cast<LPARAM>(cf));
}

DWORD CRichEdit::SetEventMask(DWORD dwFlags) {
  return ::SendMessage(m_hWindow, EM_SETEVENTMASK, 0, dwFlags);
}

void CRichEdit::SetSel(int ichStart, int ichEnd) {
  ::SendMessage(m_hWindow, EM_SETSEL, ichStart, ichEnd);
}

void CRichEdit::SetSel(CHARRANGE* cr) {
  ::SendMessage(m_hWindow, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(cr));
}