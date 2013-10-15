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
#include <windowsx.h>

namespace win32 {

// =============================================================================

void ComboBox::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = WS_EX_CLIENTEDGE;
  cs.lpszClass = WC_COMBOBOX;
  cs.style     = CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_TABSTOP | WS_VISIBLE;
}

void ComboBox::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
  Window::OnCreate(hwnd, lpCreateStruct);
}

// =============================================================================

int ComboBox::AddItem(LPCWSTR lpsz, LPARAM data) {
  int index = ComboBox_AddString(m_hWindow, lpsz);
  return ComboBox_SetItemData(m_hWindow, index, data);
}

int ComboBox::AddString(LPCWSTR lpsz) {
  return ComboBox_AddString(m_hWindow, lpsz);
}

int ComboBox::DeleteString(int index) {
  return ComboBox_DeleteString(m_hWindow, index);
}

int ComboBox::GetCount() {
  return ComboBox_GetCount(m_hWindow);
}

int ComboBox::GetCurSel() {
  return ComboBox_GetCurSel(m_hWindow);
}

LRESULT ComboBox::GetItemData(int index) {
  return ComboBox_GetItemData(m_hWindow, index);
}

void ComboBox::ResetContent() {
  ComboBox_ResetContent(m_hWindow);
}

BOOL ComboBox::SetCueBannerText(LPCWSTR lpcwText) {
  return ComboBox_SetCueBannerText(m_hWindow, lpcwText);
}

int ComboBox::SetCurSel(int index) {
  return ComboBox_SetCurSel(m_hWindow, index);
}

BOOL ComboBox::SetEditSel(int ichStart, int ichEnd) {
  return ::SendMessage(m_hWindow, CB_SETEDITSEL, ichStart, ichEnd);
}

int ComboBox::SetItemData(int index, LPARAM data) {
  return ComboBox_SetItemData(m_hWindow, index, data);
}

} // namespace win32