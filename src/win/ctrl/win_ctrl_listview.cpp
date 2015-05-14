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

ListView::ListView()
    : sort_column_(-1),
      sort_order_(1),
      sort_type_(0) {
}

ListView::ListView(HWND hwnd)
    : sort_column_(-1),
      sort_order_(1),
      sort_type_(0) {
  SetWindowHandle(hwnd);
}

void ListView::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = WS_EX_CLIENTEDGE;
  cs.lpszClass = WC_LISTVIEW;
  cs.style = WS_CHILD | WS_TABSTOP | WS_VISIBLE |
             LVS_ALIGNLEFT | LVS_AUTOARRANGE | LVS_REPORT |
             LVS_SHAREIMAGELISTS | LVS_SINGLESEL;
}

void ListView::OnCreate(HWND hwnd, LPCREATESTRUCT create_struct) {
  ListView_SetExtendedListViewStyle(hwnd,
      LVS_EX_AUTOSIZECOLUMNS | LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT |
      LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  Window::OnCreate(hwnd, create_struct);
}

////////////////////////////////////////////////////////////////////////////////

int ListView::InsertColumn(int index, int width, int width_min, int align,
                           LPCWSTR text) {
  LVCOLUMN lvc = {0};
  lvc.cx = width;
  lvc.cxMin = width_min;
  lvc.fmt = align;
  lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH |
             (width_min ? LVCF_MINWIDTH : 0);
  lvc.pszText = const_cast<LPWSTR>(text);

  if (GetVersion() < kVersionVista)
    lvc.cx = lvc.cxMin;

  return ListView_InsertColumn(window_, index, &lvc);
}

////////////////////////////////////////////////////////////////////////////////

int ListView::EnableGroupView(bool enable) {
  return ListView_EnableGroupView(window_, enable);
}

int ListView::InsertGroup(int index, LPCWSTR text,
                          bool collapsable, bool collapsed) {
  LVGROUP lvg = {0};
  lvg.cbSize = sizeof(LVGROUP);
  lvg.iGroupId = index;
  lvg.mask = LVGF_HEADER | LVGF_GROUPID;
  lvg.pszHeader = const_cast<LPWSTR>(text);

  if (collapsable && GetVersion() >= kVersionVista) {
    lvg.mask |= LVGF_STATE;
    lvg.state = LVGS_COLLAPSIBLE;
    if (collapsed)
      lvg.state |= LVGS_COLLAPSED;
  }

  return ListView_InsertGroup(window_, index, &lvg);
}

BOOL ListView::IsGroupViewEnabled() {
  return ListView_IsGroupViewEnabled(window_);
}

int ListView::SetGroupText(int index, LPCWSTR text) {
  LVGROUP lvg = {0};
  lvg.cbSize  = sizeof(LVGROUP);
  lvg.mask = LVGF_HEADER;
  lvg.pszHeader = const_cast<LPWSTR>(text);

  return ListView_SetGroupInfo(window_, index, &lvg);
}

////////////////////////////////////////////////////////////////////////////////

HIMAGELIST ListView::CreateDragImage(int item, LPPOINT pt_up_left) {
  return ListView_CreateDragImage(window_, item, pt_up_left);
}

void ListView::DeleteAllColumns() {
  while (DeleteColumn(0) == TRUE);
}

BOOL ListView::DeleteAllItems() {
  return ListView_DeleteAllItems(window_);
}

BOOL ListView::DeleteColumn(int index) {
  return ListView_DeleteColumn(window_, index);
}

BOOL ListView::DeleteItem(int item) {
  return ListView_DeleteItem(window_, item);
}

BOOL ListView::EnsureVisible(int item) {
  return ListView_EnsureVisible(window_, item, false);
}

BOOL ListView::GetCheckState(UINT index) {
  return ListView_GetCheckState(window_, index);
}

BOOL ListView::GetColumnOrderArray(int count, int* array) {
  return ListView_GetColumnOrderArray(window_, count, array);
}

int ListView::GetCountPerPage() {
  return ListView_GetCountPerPage(window_);
}

HWND ListView::GetHeader() {
  return ListView_GetHeader(window_);
}

int ListView::GetItemCount() {
  return ListView_GetItemCount(window_);
}

int ListView::GetItemGroup(int i) {
  LVITEM lvi = {0};
  lvi.iItem = i;
  lvi.mask = LVIF_GROUPID;

  if (ListView_GetItem(window_, &lvi)) {
    return lvi.iGroupId;
  } else {
    return -1;
  }
}

LPARAM ListView::GetItemParam(int i) {
  LVITEM lvi = {0};
  lvi.iItem  = i;
  lvi.mask   = LVIF_PARAM;

  if (ListView_GetItem(window_, &lvi)) {
    return lvi.lParam;
  } else {
    return 0;
  }
}

void ListView::GetItemText(int item, int subitem,
                           LPWSTR output, int max_length) {
  ListView_GetItemText(window_, item, subitem, output, max_length);
}

void ListView::GetItemText(int item, int subitem,
                           std::wstring& output, int max_length) {
  std::vector<wchar_t> buffer(max_length);
  ListView_GetItemText(window_, item, subitem, &buffer[0], max_length);
  output.assign(&buffer[0]);
}

INT ListView::GetNextItem(int start, UINT flags) {
  return ListView_GetNextItem(window_, start, flags);
}

INT ListView::GetNextItemIndex(int item, int group, LPARAM flags) {
  LVITEMINDEX lvii;
  lvii.iItem = item;
  lvii.iGroup = group;

  if (ListView_GetNextItemIndex(window_, &lvii, flags)) {
    return lvii.iItem;
  } else {
    return -1;
  }
}

UINT ListView::GetSelectedCount() {
  return ListView_GetSelectedCount(window_);
}

INT ListView::GetSelectionMark() {
  return ListView_GetSelectionMark(window_);
}

BOOL ListView::GetSubItemRect(int item, int subitem, LPRECT rect) {
  return ListView_GetSubItemRect(window_, item, subitem, LVIR_BOUNDS, rect);
}

int ListView::GetTopIndex() {
  return ListView_GetTopIndex(window_);
}

DWORD ListView::GetView() {
  return ListView_GetView(window_);
}

int ListView::HitTest(bool return_subitem) {
  LVHITTESTINFO lvhi;
  ::GetCursorPos(&lvhi.pt);
  ::ScreenToClient(window_, &lvhi.pt);
  ListView_SubItemHitTestEx(window_, &lvhi);

  return return_subitem ? lvhi.iSubItem : lvhi.iItem;
}

int ListView::HitTestEx(LPLVHITTESTINFO lplvhi) {
  ::GetCursorPos(&lplvhi->pt);
  ::ScreenToClient(window_, &lplvhi->pt);
  ListView_SubItemHitTestEx(window_, lplvhi);

  return lplvhi->iItem;
}

int ListView::InsertItem(const LVITEM& lvi) {
  return ListView_InsertItem(window_, &lvi);
}

int ListView::InsertItem(int item, int group, int image, UINT column_count,
                         PUINT columns, LPCWSTR text, LPARAM lParam) {
  LVITEM lvi = {0};
  lvi.cColumns = column_count;
  lvi.iGroupId = group;
  lvi.iImage = image;
  lvi.iItem = item;
  lvi.lParam = lParam;
  lvi.puColumns = columns;
  lvi.pszText = const_cast<LPWSTR>(text);

  if (column_count != 0)
    lvi.mask |= LVIF_COLUMNS;
  if (group > -1)
    lvi.mask |= LVIF_GROUPID;
  if (image > -1)
    lvi.mask |= LVIF_IMAGE;
  if (lParam != 0)
    lvi.mask |= LVIF_PARAM;
  if (text != nullptr)
    lvi.mask |= LVIF_TEXT;

  return ListView_InsertItem(window_, &lvi);
}

UINT ListView::IsItemVisible(UINT index) {
  return ListView_IsItemVisible(window_, index);
}

BOOL ListView::RedrawItems(int first, int last, bool repaint) {
  BOOL return_value = ListView_RedrawItems(window_, first, last);

  if (return_value && repaint)
    ::UpdateWindow(window_);

  return return_value;
}

void ListView::RemoveAllGroups() {
  ListView_RemoveAllGroups(window_);
}

void ListView::SelectAllItems(bool selected) {
  SelectItem(-1, selected);
}

void ListView::SelectItem(int index, bool selected) {
  SetItemState(index, selected ? LVIS_SELECTED : 0, LVIS_SELECTED);
}

BOOL ListView::SetBkImage(HBITMAP bitmap, ULONG flags,
                          int offset_x, int offset_y) {
  LVBKIMAGE bki= {0};
  bki.hbm = bitmap;
  bki.ulFlags = flags;
  bki.xOffsetPercent = offset_x;
  bki.yOffsetPercent = offset_y;

  return ListView_SetBkImage(window_, &bki);
}

void ListView::SetCheckState(int index, BOOL check) {
  UINT state = INDEXTOSTATEIMAGEMASK((check == TRUE) ? 2 : 1);
  ListView_SetItemState(window_, index, state, LVIS_STATEIMAGEMASK);
}

BOOL ListView::SetColumnOrderArray(int count, int* order_array) {
  return ListView_SetColumnOrderArray(window_, count, order_array);
}

BOOL ListView::SetColumnWidth(int column, int cx) {
  // LVSCW_AUTOSIZE or LVSCW_AUTOSIZE_USEHEADER can be used as cx
  return ListView_SetColumnWidth(window_, column, cx);
}

void ListView::SetExtendedStyle(DWORD ex_style) {
  ListView_SetExtendedListViewStyle(window_, ex_style);
}

DWORD ListView::SetHoverTime(DWORD hover_time) {
  return SendMessage(LVM_SETHOVERTIME, 0, hover_time);
}

void ListView::SetImageList(HIMAGELIST image_list, int type) {
  ListView_SetImageList(window_, image_list, type);
}

BOOL ListView::SetItem(int index, int subitem, LPCWSTR text) {
  LVITEM lvi = {0};
  lvi.iItem = index;
  lvi.iSubItem = subitem;
  lvi.mask = LVIF_TEXT;
  lvi.pszText = const_cast<LPWSTR>(text);

  return ListView_SetItem(window_, &lvi);
}

BOOL ListView::SetItemIcon(int index, int icon) {
  LVITEM lvi = {0};
  lvi.iImage = icon;
  lvi.iItem = index;
  lvi.mask = LVIF_IMAGE;

  return ListView_SetItem(window_, &lvi);
}

BOOL ListView::SetItemIcon(int index, int subitem, int icon) {
  LVITEM lvi = {0};
  lvi.iImage = icon;
  lvi.iItem = index;
  lvi.iSubItem = subitem;
  lvi.mask = LVIF_IMAGE;

  return ListView_SetItem(window_, &lvi);
}

void ListView::SetItemState(int index, UINT state, UINT mask) {
  ListView_SetItemState(window_, index, state, mask);
}

BOOL ListView::SetTileViewInfo(PLVTILEVIEWINFO tvi) {
  return ListView_SetTileViewInfo(window_, tvi);
}

BOOL ListView::SetTileViewInfo(int line_count, DWORD flags,
                               RECT* rc_label_margin, SIZE* size_tile) {
  LVTILEVIEWINFO tvi = {0};
  tvi.cbSize = sizeof(LVTILEVIEWINFO);

  tvi.dwFlags = flags;

  if (line_count) {
    tvi.dwMask |= LVTVIM_COLUMNS;
    tvi.cLines = line_count;
  }
  if (size_tile) {
    tvi.dwMask |= LVTVIM_TILESIZE;
    tvi.sizeTile = *size_tile;
  }
  if (rc_label_margin) {
    tvi.dwMask |= LVTVIM_LABELMARGIN;
    tvi.rcLabelMargin = *rc_label_margin;
  }

  return ListView_SetTileViewInfo(window_, &tvi);
}

int ListView::SetView(DWORD view) {
  return ListView_SetView(window_, view);
}

////////////////////////////////////////////////////////////////////////////////

int ListView::GetSortColumn() {
  return sort_column_;
}

int ListView::GetSortOrder() {
  return sort_order_;
}

int ListView::GetSortType() {
  return sort_type_;
}

void ListView::Sort(int sort_column, int sort_order, int type,
                    PFNLVCOMPARE compare) {
  sort_column_ = sort_column;
  sort_order_ = (sort_order == 0) ? 1 : sort_order;
  sort_type_ = type;

  ListView_SortItemsEx(window_, compare, this);

  if (GetVersion() < kVersionVista) {
    HDITEM hdi = {0};
    hdi.mask = HDI_FORMAT;

    HWND header = ListView_GetHeader(window_);
    for (int i = 0; i < Header_GetItemCount(header); ++i) {
      Header_GetItem(header, i, &hdi);
      hdi.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
      Header_SetItem(header, i, &hdi);
    }

    Header_GetItem(header, sort_column, &hdi);
    hdi.fmt |= (sort_order > -1 ? HDF_SORTUP : HDF_SORTDOWN);
    Header_SetItem(header, sort_column, &hdi);
  }
}

}  // namespace win