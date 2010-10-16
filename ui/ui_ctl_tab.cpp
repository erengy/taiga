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

void CTab::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = NULL;
  cs.lpszClass = WC_TABCONTROL;
  cs.style     = WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_TOOLTIPS;
}

BOOL CTab::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
  TabCtrl_SetExtendedStyle(hwnd, TCS_EX_REGISTERDROP);
  return TRUE;
}

// =============================================================================

void CTab::AdjustRect(BOOL fLarger, LPRECT lpRect) {
  TabCtrl_AdjustRect(m_hWindow, fLarger, lpRect);
}

int CTab::Clear() {
  return TabCtrl_DeleteAllItems(m_hWindow);
}

int CTab::DeleteItem(int nIndex) {
  return TabCtrl_DeleteItem(m_hWindow, nIndex);
}

int CTab::InsertItem(int nIndex, LPCWSTR szText, LPARAM lParam) {
  TCITEM tci;
  tci.mask    = TCIF_PARAM | TCIF_TEXT;
  tci.pszText = (LPWSTR)szText;
  tci.lParam  = lParam;
  tci.iImage  = -1;

  return TabCtrl_InsertItem(m_hWindow, nIndex, &tci);
}

int CTab::GetCurrentlySelected() {
  return TabCtrl_GetCurSel(m_hWindow);
}

LPARAM CTab::GetItemParam(int nIndex) {
  TCITEM tci;
  tci.mask = TCIF_PARAM;
  TabCtrl_GetItem(m_hWindow, nIndex, &tci);
  return tci.lParam;
}

int CTab::HitTest() {
  TCHITTESTINFO tchti;
  ::GetCursorPos(&tchti.pt);
  ::ScreenToClient(m_hWindow, &tchti.pt);
  return TabCtrl_HitTest(m_hWindow, &tchti);
}

int CTab::SetCurrentlySelected(int iItem) {
  return TabCtrl_SetCurSel(m_hWindow, iItem);
}

int CTab::SetItemText(int iItem, LPCWSTR szText) {
  TCITEM tci;
  tci.mask = TCIF_TEXT;
  tci.pszText = (LPWSTR)szText;

  return TabCtrl_SetItem(m_hWindow, iItem, &tci);
}