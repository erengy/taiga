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

void CTreeView::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = WS_EX_CLIENTEDGE;
  cs.lpszClass = WC_TREEVIEW;
  cs.style     = WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;
}

BOOL CTreeView::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
  TreeView_SetExtendedStyle(hwnd, TVS_EX_DOUBLEBUFFER, NULL);
  return TRUE;
}

// =============================================================================

BOOL CTreeView::DeleteAllItems() {
  return TreeView_DeleteAllItems(m_hWindow);
}

BOOL CTreeView::Expand(HTREEITEM hItem, bool bExpand) {
  return TreeView_Expand(m_hWindow, hItem, bExpand ? TVE_EXPAND : TVE_COLLAPSE);
}

UINT CTreeView::GetCount() {
  return TreeView_GetCount(m_hWindow);
}

BOOL CTreeView::GetItem(LPTVITEM pItem) {
  return TreeView_GetItem(m_hWindow, pItem);
}

HTREEITEM CTreeView::InsertItem(LPCWSTR pszText, LPARAM lParam, HTREEITEM htiParent) {
  TVITEM tvi  = {0};
  tvi.mask    = TVIF_TEXT | TVIF_PARAM;
  tvi.pszText = (LPWSTR)pszText;
  tvi.lParam  = lParam;

  TVINSERTSTRUCT tvis = {0};
  tvis.item           = tvi; 
  tvis.hInsertAfter   = 0;
  tvis.hParent        = htiParent;

  return TreeView_InsertItem(m_hWindow, &tvis);
}

BOOL CTreeView::SelectItem(HTREEITEM hItem) {
  return TreeView_Select(m_hWindow, hItem, TVGN_CARET);
}

int CTreeView::SetItemHeight(SHORT cyItem) {
  return TreeView_SetItemHeight(m_hWindow, cyItem);
}