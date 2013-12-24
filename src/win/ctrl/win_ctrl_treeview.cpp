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

void TreeView::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = WS_EX_CLIENTEDGE;
  cs.lpszClass = WC_TREEVIEW;
  cs.style     = WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;
}

void TreeView::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
  TreeView_SetExtendedStyle(hwnd, TVS_EX_DOUBLEBUFFER, NULL);
  Window::OnCreate(hwnd, lpCreateStruct);
}

// =============================================================================

BOOL TreeView::DeleteAllItems() {
  return TreeView_DeleteAllItems(window_);
}

BOOL TreeView::DeleteItem(HTREEITEM hitem) {
  return TreeView_DeleteItem(window_, hitem);
}

BOOL TreeView::Expand(HTREEITEM hItem, bool bExpand) {
  return TreeView_Expand(window_, hItem, bExpand ? TVE_EXPAND : TVE_COLLAPSE);
}

UINT TreeView::GetCheckState(HTREEITEM hItem) {
  return TreeView_GetCheckState(window_, hItem);
}

UINT TreeView::GetCount() {
  return TreeView_GetCount(window_);
}

BOOL TreeView::GetItem(LPTVITEM pItem) {
  return TreeView_GetItem(window_, pItem);
}

LPARAM TreeView::GetItemData(HTREEITEM hItem) {
  TVITEM tvi = {0};
  tvi.mask = TVIF_PARAM;
  tvi.hItem = hItem;
  TreeView_GetItem(window_, &tvi);

  return tvi.lParam;
}

HTREEITEM TreeView::GetSelection() {
  return TreeView_GetSelection(window_);
}

HTREEITEM TreeView::HitTest(LPTVHITTESTINFO lpht, bool bGetCursorPos) {
  if (bGetCursorPos) {
    GetCursorPos(&lpht->pt);
    ScreenToClient(window_, &lpht->pt);
  }
  return TreeView_HitTest(window_, lpht);
}

HTREEITEM TreeView::InsertItem(LPCWSTR pszText, int iImage, LPARAM lParam, HTREEITEM htiParent, HTREEITEM hInsertAfter) {
  TVITEM tvi  = {0};
  tvi.mask    = TVIF_TEXT | TVIF_PARAM;
  tvi.pszText = (LPWSTR)pszText;
  tvi.lParam  = lParam;

  if (iImage > -1) {
    tvi.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.iImage = iImage;
    tvi.iSelectedImage = iImage;
  }

  TVINSERTSTRUCT tvis = {0};
  tvis.item           = tvi;
  tvis.hInsertAfter   = hInsertAfter;
  tvis.hParent        = htiParent;

  return TreeView_InsertItem(window_, &tvis);
}

BOOL TreeView::SelectItem(HTREEITEM hItem) {
  return TreeView_Select(window_, hItem, TVGN_CARET);
}

UINT TreeView::SetCheckState(HTREEITEM hItem, BOOL fCheck) {
  TVITEM tvi    = {0};
  tvi.mask      = TVIF_HANDLE | TVIF_STATE;
  tvi.hItem     = hItem;
  tvi.stateMask = TVIS_STATEIMAGEMASK;
  tvi.state     = INDEXTOSTATEIMAGEMASK(fCheck + 1);

  return TreeView_SetItem(window_, &tvi);
}

HIMAGELIST TreeView::SetImageList(HIMAGELIST himl, INT iImage) {
  return TreeView_SetImageList(window_, himl, iImage);
}

BOOL TreeView::SetItem(HTREEITEM hItem, LPCWSTR pszText) {
  TVITEM tvi = {0};
  tvi.mask   = TVIF_HANDLE;
  tvi.hItem  = hItem;

  if (!TreeView_GetItem(window_, &tvi))
    return FALSE;

  tvi.mask |= TVIF_TEXT;
  tvi.pszText = (LPWSTR)pszText;
  return TreeView_SetItem(window_, &tvi);
}

int TreeView::SetItemHeight(SHORT cyItem) {
  return TreeView_SetItemHeight(window_, cyItem);
}

}  // namespace win