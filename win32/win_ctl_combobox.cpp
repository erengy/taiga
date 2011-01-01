/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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

#include <windowsx.h>
#include "win_control.h"

// =============================================================================

void CComboBox::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = WS_EX_CLIENTEDGE;
  cs.lpszClass = WC_COMBOBOX;
  cs.style     = CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_TABSTOP | WS_VISIBLE;
}

BOOL CComboBox::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
  return TRUE;
}

// =============================================================================

int CComboBox::AddString(LPCWSTR lpsz) {
  return ComboBox_AddString(m_hWindow, lpsz);
}

int CComboBox::DeleteString(int index) {
  return ComboBox_DeleteString(m_hWindow, index);
}

int CComboBox::GetCount() {
  return ComboBox_GetCount(m_hWindow);
}

int CComboBox::GetCurSel() {
  return ComboBox_GetCurSel(m_hWindow);
}

LRESULT CComboBox::GetItemData(int index) {
  return ComboBox_GetItemData(m_hWindow, index);
}

void CComboBox::ResetContent() {
  ComboBox_ResetContent(m_hWindow);
}

int CComboBox::SetCurSel(int index) {
  return ComboBox_SetCurSel(m_hWindow, index);
}

BOOL CComboBox::SetEditSel(int ichStart, int ichEnd) {
  return ::SendMessage(m_hWindow, CB_SETEDITSEL, ichStart, ichEnd);
}

int CComboBox::SetItemData(int index, LPARAM data) {
  return ComboBox_SetItemData(m_hWindow, index, data);
}