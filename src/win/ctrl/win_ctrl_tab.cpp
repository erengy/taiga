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

void Tab::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = NULL;
  cs.lpszClass = WC_TABCONTROL;
  cs.style     = WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_TOOLTIPS;
}

void Tab::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
  TabCtrl_SetExtendedStyle(hwnd, TCS_EX_REGISTERDROP);
  Window::OnCreate(hwnd, lpCreateStruct);
}

// =============================================================================

void Tab::AdjustRect(HWND hWindow, BOOL fLarger, LPRECT lpRect) {
  if (hWindow) {
    RECT window_rect;
    ::GetClientRect(window_, lpRect);
    ::GetWindowRect(window_, &window_rect);
    ::ScreenToClient(hWindow, (LPPOINT)&window_rect);
    ::SetRect(lpRect, 
      window_rect.left, window_rect.top, 
      window_rect.left + lpRect->right, 
      window_rect.top + lpRect->bottom);
  }
  TabCtrl_AdjustRect(window_, fLarger, lpRect);
}

int Tab::DeleteAllItems() {
  return TabCtrl_DeleteAllItems(window_);
}

int Tab::DeleteItem(int nIndex) {
  return TabCtrl_DeleteItem(window_, nIndex);
}

int Tab::InsertItem(int nIndex, LPCWSTR szText, LPARAM lParam) {
  TCITEM tci;
  tci.mask    = TCIF_PARAM | TCIF_TEXT;
  tci.pszText = (LPWSTR)szText;
  tci.lParam  = lParam;
  tci.iImage  = -1;

  return TabCtrl_InsertItem(window_, nIndex, &tci);
}

int Tab::GetCurrentlySelected() {
  return TabCtrl_GetCurSel(window_);
}

int Tab::GetItemCount() {
  return TabCtrl_GetItemCount(window_);
}

LPARAM Tab::GetItemParam(int nIndex) {
  TCITEM tci;
  tci.mask = TCIF_PARAM;
  TabCtrl_GetItem(window_, nIndex, &tci);
  return tci.lParam;
}

int Tab::HitTest() {
  TCHITTESTINFO tchti;
  ::GetCursorPos(&tchti.pt);
  ::ScreenToClient(window_, &tchti.pt);
  return TabCtrl_HitTest(window_, &tchti);
}

int Tab::SetCurrentlySelected(int iItem) {
  return TabCtrl_SetCurSel(window_, iItem);
}

int Tab::SetItemText(int iItem, LPCWSTR szText) {
  TCITEM tci;
  tci.mask = TCIF_TEXT;
  tci.pszText = (LPWSTR)szText;

  return TabCtrl_SetItem(window_, iItem, &tci);
}

}  // namespace win