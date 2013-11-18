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

ListView::ListView() {
  m_iSortColumn = -1;
  m_iSortOrder = 1;
  m_iSortType = 0;
}

// =============================================================================

void ListView::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = WS_EX_CLIENTEDGE;
  cs.lpszClass = WC_LISTVIEW;
  cs.style     = WS_CHILD | WS_TABSTOP | WS_VISIBLE | LVS_ALIGNLEFT | LVS_AUTOARRANGE | 
                 LVS_REPORT | LVS_SHAREIMAGELISTS | LVS_SINGLESEL;
}

void ListView::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
  ListView_SetExtendedListViewStyle(hwnd, LVS_EX_AUTOSIZECOLUMNS | LVS_EX_DOUBLEBUFFER | 
    LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  Window::OnCreate(hwnd, lpCreateStruct);
}

// =============================================================================

int ListView::InsertColumn(int nIndex, int nWidth, int nWidthMin, int nAlign, LPCWSTR szText) {
  LVCOLUMN lvc = {0};
  lvc.cx       = nWidth;
  lvc.cxMin    = nWidthMin;
  lvc.fmt      = nAlign;
  lvc.mask     = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | (nWidthMin ? LVCF_MINWIDTH : NULL);
  lvc.pszText  = const_cast<LPWSTR>(szText);

  if (GetWinVersion() < VERSION_VISTA) {
    lvc.cx = lvc.cxMin;
  }

  return ListView_InsertColumn(m_hWindow, nIndex, &lvc);
}

// =============================================================================

int ListView::EnableGroupView(bool bValue) {
  return ListView_EnableGroupView(m_hWindow, bValue);
}

int ListView::InsertGroup(int nIndex, LPCWSTR szText, bool bCollapsable, bool bCollapsed) {
  LVGROUP lvg = {0};
  lvg.cbSize    = sizeof(LVGROUP);
  lvg.iGroupId  = nIndex;
  lvg.mask      = LVGF_HEADER | LVGF_GROUPID;
  lvg.pszHeader = const_cast<LPWSTR>(szText);
  
  if (bCollapsable && GetWinVersion() >= VERSION_VISTA) {
    lvg.mask |= LVGF_STATE;
    lvg.state = LVGS_COLLAPSIBLE;
    if (bCollapsed) lvg.state |= LVGS_COLLAPSED;
  }
  
  return ListView_InsertGroup(m_hWindow, nIndex, &lvg);
}

BOOL ListView::IsGroupViewEnabled() {
  return ListView_IsGroupViewEnabled(m_hWindow);
}

int ListView::SetGroupText(int nIndex, LPCWSTR szText) {
  LVGROUP lvg = {0};
  lvg.cbSize  = sizeof(LVGROUP);
  lvg.mask      = LVGF_HEADER;
  lvg.pszHeader = const_cast<LPWSTR>(szText);
  return ListView_SetGroupInfo(m_hWindow, nIndex, &lvg);
}

// =============================================================================

HIMAGELIST ListView::CreateDragImage(int iItem, LPPOINT lpptUpLeft) {
  return ListView_CreateDragImage(m_hWindow, iItem, lpptUpLeft);
}

BOOL ListView::DeleteAllItems() {
  return ListView_DeleteAllItems(m_hWindow);
}

BOOL ListView::DeleteItem(int iItem) {
  return ListView_DeleteItem(m_hWindow, iItem);
}

BOOL ListView::EnsureVisible(int i) {
  return ListView_EnsureVisible(m_hWindow, i, false);
}

BOOL ListView::GetCheckState(UINT iIndex) {
  return ListView_GetCheckState(m_hWindow, iIndex);
}

HWND ListView::GetHeader() {
  return ListView_GetHeader(m_hWindow);
}

int ListView::GetItemCount() {
  return ListView_GetItemCount(m_hWindow);
}

LPARAM ListView::GetItemParam(int i) {  
  LVITEM lvi = {0};
  lvi.iItem  = i;
  lvi.mask   = LVIF_PARAM;

  if (ListView_GetItem(m_hWindow, &lvi)) {
    return lvi.lParam;
  } else {
    return NULL;
  }
}

void ListView::GetItemText(int iItem, int iSubItem, LPWSTR pszText, int cchTextMax) {
  ListView_GetItemText(m_hWindow, iItem, iSubItem, pszText, cchTextMax);
}

void ListView::GetItemText(int iItem, int iSubItem, wstring& str, int cchTextMax) {
  vector<wchar_t> buffer(cchTextMax);
  ListView_GetItemText(m_hWindow, iItem, iSubItem, &buffer[0], cchTextMax);
  str.assign(&buffer[0]);
}

INT ListView::GetNextItem(int iStart, UINT flags) {
  return ListView_GetNextItem(m_hWindow, iStart, flags);
}

INT ListView::GetNextItemIndex(int iItem, int iGroup, LPARAM flags) {
  LVITEMINDEX lvii;
  lvii.iItem = iItem;
  lvii.iGroup = iGroup;
  if (ListView_GetNextItemIndex(m_hWindow, &lvii, flags)) {
    return lvii.iItem;
  } else {
    return -1;
  }
}

UINT ListView::GetSelectedCount() {
  return ListView_GetSelectedCount(m_hWindow);
}

INT ListView::GetSelectionMark() {
  return ListView_GetSelectionMark(m_hWindow);
}

BOOL ListView::GetSubItemRect(int iItem, int iSubItem, LPRECT lpRect) {
  return ListView_GetSubItemRect(m_hWindow, iItem, iSubItem, LVIR_BOUNDS, lpRect);
}

DWORD ListView::GetView() {
  return ListView_GetView(m_hWindow);
}

int ListView::HitTest(bool return_subitem) {
  LVHITTESTINFO lvhi;
  ::GetCursorPos(&lvhi.pt);
  ::ScreenToClient(m_hWindow, &lvhi.pt);
  ListView_SubItemHitTestEx(m_hWindow, &lvhi);
  return return_subitem ? lvhi.iSubItem : lvhi.iItem;
}

int ListView::HitTestEx(LPLVHITTESTINFO lplvhi) {
  ::GetCursorPos(&lplvhi->pt);
  ::ScreenToClient(m_hWindow, &lplvhi->pt);
  ListView_SubItemHitTestEx(m_hWindow, lplvhi);
  return lplvhi->iItem;
}

int ListView::InsertItem(const LVITEM& lvi) {
  return ListView_InsertItem(m_hWindow, &lvi);
}

int ListView::InsertItem(int iItem, int iGroupId, int iImage, UINT cColumns, PUINT puColumns, LPCWSTR pszText, LPARAM lParam) {
  LVITEM lvi    = {0};
  lvi.cColumns  = cColumns;
  lvi.iGroupId  = iGroupId;
  lvi.iImage    = iImage;
  lvi.iItem     = iItem;
  lvi.lParam    = lParam;
  lvi.puColumns = puColumns;
  lvi.pszText   = const_cast<LPWSTR>(pszText);

  if (cColumns != 0)   lvi.mask |= LVIF_COLUMNS;
  if (iGroupId > -1)   lvi.mask |= LVIF_GROUPID;
  if (iImage > -1)     lvi.mask |= LVIF_IMAGE;
  if (lParam != 0)     lvi.mask |= LVIF_PARAM;
  if (pszText != NULL) lvi.mask |= LVIF_TEXT;

  return ListView_InsertItem(m_hWindow, &lvi);
}

BOOL ListView::RedrawItems(int iFirst, int iLast, bool repaint) {
  BOOL return_value = ListView_RedrawItems(m_hWindow, iFirst, iLast);
  if (return_value && repaint) ::UpdateWindow(m_hWindow);
  return return_value;
}

void ListView::RemoveAllGroups() {
  ListView_RemoveAllGroups(m_hWindow);
}

BOOL ListView::SetBkImage(HBITMAP hbmp, ULONG ulFlags, int xOffset, int yOffset) {
  LVBKIMAGE bki      = {0};
  bki.hbm            = hbmp;
  bki.ulFlags        = ulFlags;
  bki.xOffsetPercent = xOffset;
  bki.yOffsetPercent = yOffset;
  return ListView_SetBkImage(m_hWindow, &bki);
}

void ListView::SetCheckState(int iIndex, BOOL fCheck) {
  ListView_SetItemState(m_hWindow, iIndex, INDEXTOSTATEIMAGEMASK((fCheck==TRUE) ? 2 : 1), LVIS_STATEIMAGEMASK);
}

BOOL ListView::SetColumnWidth(int iCol, int cx) {
  // LVSCW_AUTOSIZE or LVSCW_AUTOSIZE_USEHEADER can be used as cx
  return ListView_SetColumnWidth(m_hWindow, iCol, cx);
}

void ListView::SetExtendedStyle(DWORD dwExStyle) {
  ListView_SetExtendedListViewStyle(m_hWindow, dwExStyle);
}

DWORD ListView::SetHoverTime(DWORD dwHoverTime) {
  return ::SendMessage(m_hWindow, LVM_SETHOVERTIME, 0, dwHoverTime);
}

void ListView::SetImageList(HIMAGELIST hImageList, int iImageList) {
  ListView_SetImageList(m_hWindow, hImageList, iImageList);
}

BOOL ListView::SetItem(int nIndex, int nSubItem, LPCWSTR szText) {
  LVITEM lvi   = {0};
  lvi.iItem    = nIndex;
  lvi.iSubItem = nSubItem;
  lvi.mask     = LVIF_TEXT;
  lvi.pszText  = const_cast<LPWSTR>(szText);

  return ListView_SetItem(m_hWindow, &lvi);
}

BOOL ListView::SetItemIcon(int nIndex, int nIcon) {
  LVITEM lvi = {0};
  lvi.iImage = nIcon;
  lvi.iItem  = nIndex;
  lvi.mask   = LVIF_IMAGE;

  return ListView_SetItem(m_hWindow, &lvi);
}

void ListView::SetSelectedItem(int iIndex) {
  ListView_SetItemState(m_hWindow, iIndex, LVIS_SELECTED, LVIS_SELECTED);
}

BOOL ListView::SetTileViewInfo(PLVTILEVIEWINFO plvtvinfo) {
  return ListView_SetTileViewInfo(m_hWindow, plvtvinfo);
}

BOOL ListView::SetTileViewInfo(int cLines, DWORD dwFlags, RECT* rcLabelMargin, SIZE* sizeTile) {
  LVTILEVIEWINFO tvi = {0};
  tvi.cbSize = sizeof(LVTILEVIEWINFO);
  
  tvi.dwFlags = dwFlags;

  if (cLines) {
    tvi.dwMask |= LVTVIM_COLUMNS;
    tvi.cLines = cLines;
  }
  if (sizeTile) {
    tvi.dwMask |= LVTVIM_TILESIZE;
    tvi.sizeTile = *sizeTile;
  }
  if (rcLabelMargin) {
    tvi.dwMask |= LVTVIM_LABELMARGIN;
    tvi.rcLabelMargin = *rcLabelMargin;
  }

  return ListView_SetTileViewInfo(m_hWindow, &tvi);
}

int ListView::SetView(DWORD iView) {
  return ListView_SetView(m_hWindow, iView);
}

// =============================================================================

int ListView::GetSortColumn() { return m_iSortColumn; }
int ListView::GetSortOrder() { return m_iSortOrder; }
int ListView::GetSortType() { return m_iSortType; }

void ListView::Sort(int iColumn, int iOrder, int iType, PFNLVCOMPARE pfnCompare) {
  m_iSortColumn = iColumn;
  m_iSortOrder = (iOrder == 0)? 1 : iOrder;
  m_iSortType = iType;

  ListView_SortItemsEx(m_hWindow, pfnCompare, this);

  if (GetWinVersion() < VERSION_VISTA) {
    HDITEM hdi   = {0};
    hdi.mask     = HDI_FORMAT;
    HWND hHeader = ListView_GetHeader(m_hWindow);
    for (int i = 0; i < Header_GetItemCount(hHeader); ++i) {
      Header_GetItem(hHeader, i, &hdi);
      hdi.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
      Header_SetItem(hHeader, i, &hdi);
    }
    Header_GetItem(hHeader, iColumn, &hdi);
    hdi.fmt |= (iOrder > -1 ? HDF_SORTUP : HDF_SORTDOWN);
    Header_SetItem(hHeader, iColumn, &hdi);
  }
}

} // namespace win32