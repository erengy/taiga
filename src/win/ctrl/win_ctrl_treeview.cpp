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

TreeView::TreeView(HWND hwnd) {
  SetWindowHandle(hwnd);
}

void TreeView::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = WS_EX_CLIENTEDGE;
  cs.lpszClass = WC_TREEVIEW;
  cs.style = WS_CHILD | WS_VISIBLE | WS_TABSTOP |
             TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;
}

void TreeView::OnCreate(HWND hwnd, LPCREATESTRUCT create_struct) {
  TreeView_SetExtendedStyle(hwnd, TVS_EX_DOUBLEBUFFER, 0);
  Window::OnCreate(hwnd, create_struct);
}

////////////////////////////////////////////////////////////////////////////////

BOOL TreeView::DeleteAllItems() {
  return TreeView_DeleteAllItems(window_);
}

BOOL TreeView::DeleteItem(HTREEITEM item) {
  return TreeView_DeleteItem(window_, item);
}

BOOL TreeView::Expand(HTREEITEM item, bool expand) {
  return TreeView_Expand(window_, item, expand ? TVE_EXPAND : TVE_COLLAPSE);
}

UINT TreeView::GetCheckState(HTREEITEM item) {
  return TreeView_GetCheckState(window_, item);
}

UINT TreeView::GetCount() {
  return TreeView_GetCount(window_);
}

BOOL TreeView::GetItem(LPTVITEM item) {
  return TreeView_GetItem(window_, item);
}

LPARAM TreeView::GetItemData(HTREEITEM item) {
  TVITEM tvi = {0};
  tvi.mask = TVIF_PARAM;
  tvi.hItem = item;
  TreeView_GetItem(window_, &tvi);

  return tvi.lParam;
}

HTREEITEM TreeView::GetNextItem(HTREEITEM item, UINT flag) {
  return TreeView_GetNextItem(window_, item, flag);
}

HTREEITEM TreeView::GetSelection() {
  return TreeView_GetSelection(window_);
}

HTREEITEM TreeView::HitTest(LPTVHITTESTINFO hti, bool get_cursor_pos) {
  if (get_cursor_pos) {
    GetCursorPos(&hti->pt);
    ScreenToClient(window_, &hti->pt);
  }

  return TreeView_HitTest(window_, hti);
}

HTREEITEM TreeView::InsertItem(LPCWSTR text, int image, LPARAM param,
                               HTREEITEM parent, HTREEITEM insert_after) {
  TVITEM tvi = {0};
  tvi.mask = TVIF_TEXT | TVIF_PARAM;
  tvi.pszText = const_cast<LPWSTR>(text);
  tvi.lParam = param;

  if (image > -1) {
    tvi.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.iImage = image;
    tvi.iSelectedImage = image;
  }

  TVINSERTSTRUCT tvis = {0};
  tvis.item = tvi;
  tvis.hInsertAfter = insert_after;
  tvis.hParent = parent;

  return TreeView_InsertItem(window_, &tvis);
}

BOOL TreeView::SelectItem(HTREEITEM item) {
  return TreeView_Select(window_, item, TVGN_CARET);
}

UINT TreeView::SetCheckState(HTREEITEM item, BOOL check) {
  TVITEM tvi = {0};
  tvi.mask = TVIF_HANDLE | TVIF_STATE;
  tvi.hItem = item;
  tvi.stateMask = TVIS_STATEIMAGEMASK;
  tvi.state = INDEXTOSTATEIMAGEMASK(check + 1);

  return TreeView_SetItem(window_, &tvi);
}

HIMAGELIST TreeView::SetImageList(HIMAGELIST image_list, INT image) {
  return TreeView_SetImageList(window_, image_list, image);
}

BOOL TreeView::SetItem(HTREEITEM item, LPCWSTR text) {
  TVITEM tvi = {0};
  tvi.mask = TVIF_HANDLE;
  tvi.hItem = item;

  if (!TreeView_GetItem(window_, &tvi))
    return FALSE;

  tvi.mask |= TVIF_TEXT;
  tvi.pszText = const_cast<LPWSTR>(text);
  return TreeView_SetItem(window_, &tvi);
}

int TreeView::SetItemHeight(SHORT cyItem) {
  return TreeView_SetItemHeight(window_, cyItem);
}

}  // namespace win