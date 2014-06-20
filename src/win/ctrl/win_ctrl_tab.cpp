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

Tab::Tab(HWND hwnd) {
  SetWindowHandle(hwnd);
}

void Tab::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = 0;
  cs.lpszClass = WC_TABCONTROL;
  cs.style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_TOOLTIPS;
}

void Tab::OnCreate(HWND hwnd, LPCREATESTRUCT create_struct) {
  TabCtrl_SetExtendedStyle(hwnd, TCS_EX_REGISTERDROP);
  Window::OnCreate(hwnd, create_struct);
}

////////////////////////////////////////////////////////////////////////////////

void Tab::AdjustRect(HWND hwnd, BOOL larger, LPRECT rect) {
  if (hwnd) {
    RECT window_rect;
    ::GetClientRect(window_, rect);
    ::GetWindowRect(window_, &window_rect);
    ::ScreenToClient(hwnd, (LPPOINT)&window_rect);
    ::SetRect(rect,
              window_rect.left,
              window_rect.top,
              window_rect.left + rect->right,
              window_rect.top + rect->bottom);
  }

  TabCtrl_AdjustRect(window_, larger, rect);
}

int Tab::DeleteAllItems() {
  return TabCtrl_DeleteAllItems(window_);
}

int Tab::DeleteItem(int index) {
  return TabCtrl_DeleteItem(window_, index);
}

int Tab::InsertItem(int index, LPCWSTR text, LPARAM param) {
  TCITEM tci;

  tci.mask = TCIF_PARAM | TCIF_TEXT;
  tci.pszText = const_cast<LPWSTR>(text);
  tci.lParam = param;
  tci.iImage = -1;

  return TabCtrl_InsertItem(window_, index, &tci);
}

int Tab::GetCurrentlySelected() {
  return TabCtrl_GetCurSel(window_);
}

int Tab::GetItemCount() {
  return TabCtrl_GetItemCount(window_);
}

LPARAM Tab::GetItemParam(int index) {
  TCITEM tci;
  tci.mask = TCIF_PARAM;
  TabCtrl_GetItem(window_, index, &tci);

  return tci.lParam;
}

int Tab::HitTest() {
  TCHITTESTINFO tchti;
  ::GetCursorPos(&tchti.pt);
  ::ScreenToClient(window_, &tchti.pt);

  return TabCtrl_HitTest(window_, &tchti);
}

int Tab::SetCurrentlySelected(int item) {
  return TabCtrl_SetCurSel(window_, item);
}

int Tab::SetItemText(int item, LPCWSTR text) {
  TCITEM tci;
  tci.mask = TCIF_TEXT;
  tci.pszText = const_cast<LPWSTR>(text);

  return TabCtrl_SetItem(window_, item, &tci);
}

}  // namespace win