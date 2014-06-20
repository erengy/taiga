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
#include <windowsx.h>

namespace win {

ComboBox::ComboBox(HWND hwnd) {
  SetWindowHandle(hwnd);
}

void ComboBox::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = WS_EX_CLIENTEDGE;
  cs.lpszClass = WC_COMBOBOX;
  cs.style = CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_TABSTOP | WS_VISIBLE;
}

void ComboBox::OnCreate(HWND hwnd, LPCREATESTRUCT create_struct) {
  Window::OnCreate(hwnd, create_struct);
}

////////////////////////////////////////////////////////////////////////////////

int ComboBox::AddItem(LPCWSTR text, LPARAM data) {
  int index = ComboBox_AddString(window_, text);
  return ComboBox_SetItemData(window_, index, data);
}

int ComboBox::AddString(LPCWSTR text) {
  return ComboBox_AddString(window_, text);
}

int ComboBox::DeleteString(int index) {
  return ComboBox_DeleteString(window_, index);
}

int ComboBox::FindItemData(LPARAM data) {
  for (int i = 0; i < GetCount(); i++)
    if (data == GetItemData(i))
      return i;

  return CB_ERR;
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

BOOL ComboBox::SetCueBannerText(LPCWSTR text) {
  return ComboBox_SetCueBannerText(window_, text);
}

int ComboBox::SetCurSel(int index) {
  return ComboBox_SetCurSel(window_, index);
}

BOOL ComboBox::SetEditSel(int start, int end) {
  return ::SendMessage(window_, CB_SETEDITSEL, start, end);
}

int ComboBox::SetItemData(int index, LPARAM data) {
  return ComboBox_SetItemData(window_, index, data);
}

}  // namespace win