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
#include <windowsx.h>

namespace win {

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
  int index = ComboBox_AddString(window_, lpsz);
  return ComboBox_SetItemData(window_, index, data);
}

int ComboBox::AddString(LPCWSTR lpsz) {
  return ComboBox_AddString(window_, lpsz);
}

int ComboBox::DeleteString(int index) {
  return ComboBox_DeleteString(window_, index);
}

int ComboBox::GetCount() {
  return ComboBox_GetCount(window_);
}

int ComboBox::GetCurSel() {
  return ComboBox_GetCurSel(window_);
}

LRESULT ComboBox::GetItemData(int index) {
  return ComboBox_GetItemData(window_, index);
}

void ComboBox::ResetContent() {
  ComboBox_ResetContent(window_);
}

BOOL ComboBox::SetCueBannerText(LPCWSTR lpcwText) {
  return ComboBox_SetCueBannerText(window_, lpcwText);
}

int ComboBox::SetCurSel(int index) {
  return ComboBox_SetCurSel(window_, index);
}

BOOL ComboBox::SetEditSel(int ichStart, int ichEnd) {
  return ::SendMessage(window_, CB_SETEDITSEL, ichStart, ichEnd);
}

int ComboBox::SetItemData(int index, LPARAM data) {
  return ComboBox_SetItemData(window_, index, data);
}

}  // namespace win