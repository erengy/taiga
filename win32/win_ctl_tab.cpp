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

namespace win32 {

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
    ::GetClientRect(m_hWindow, lpRect);
    ::GetWindowRect(m_hWindow, &window_rect);
    ::ScreenToClient(hWindow, (LPPOINT)&window_rect);
    ::SetRect(lpRect, 
      window_rect.left, window_rect.top, 
      window_rect.left + lpRect->right, 
      window_rect.top + lpRect->bottom);
  }
  TabCtrl_AdjustRect(m_hWindow, fLarger, lpRect);
}

int Tab::DeleteAllItems() {
  return TabCtrl_DeleteAllItems(m_hWindow);
}

int Tab::DeleteItem(int nIndex) {
  return TabCtrl_DeleteItem(m_hWindow, nIndex);
}

int Tab::InsertItem(int nIndex, LPCWSTR szText, LPARAM lParam) {
  TCITEM tci;
  tci.mask    = TCIF_PARAM | TCIF_TEXT;
  tci.pszText = (LPWSTR)szText;
  tci.lParam  = lParam;
  tci.iImage  = -1;

  return TabCtrl_InsertItem(m_hWindow, nIndex, &tci);
}

int Tab::GetCurrentlySelected() {
  return TabCtrl_GetCurSel(m_hWindow);
}

int Tab::GetItemCount() {
  return TabCtrl_GetItemCount(m_hWindow);
}

LPARAM Tab::GetItemParam(int nIndex) {
  TCITEM tci;
  tci.mask = TCIF_PARAM;
  TabCtrl_GetItem(m_hWindow, nIndex, &tci);
  return tci.lParam;
}

int Tab::HitTest() {
  TCHITTESTINFO tchti;
  ::GetCursorPos(&tchti.pt);
  ::ScreenToClient(m_hWindow, &tchti.pt);
  return TabCtrl_HitTest(m_hWindow, &tchti);
}

int Tab::SetCurrentlySelected(int iItem) {
  return TabCtrl_SetCurSel(m_hWindow, iItem);
}

int Tab::SetItemText(int iItem, LPCWSTR szText) {
  TCITEM tci;
  tci.mask = TCIF_TEXT;
  tci.pszText = (LPWSTR)szText;

  return TabCtrl_SetItem(m_hWindow, iItem, &tci);
}

} // namespace win32