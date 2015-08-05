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

#ifndef TAIGA_WIN_CTRL_H
#define TAIGA_WIN_CTRL_H

#include <windows.h>
#include <commctrl.h>
#include <richedit.h>

#include "win/win_main.h"
#include "win/win_window.h"

namespace win {

////////////////////////////////////////////////////////////////////////////////
// ComboBox

class ComboBox : public Window {
public:
  ComboBox() {}
  ComboBox(HWND hwnd);
  virtual ~ComboBox() {}

  int     AddItem(LPCWSTR text, LPARAM data);
  int     AddString(LPCWSTR text);
  int     DeleteString(int index);
  int     FindItemData(LPARAM data);
  int     GetCount();
  LRESULT GetItemData(int index);
  int     GetCurSel();
  void    ResetContent();
  BOOL    SetCueBannerText(LPCWSTR text);
  int     SetCurSel(int index);
  BOOL    SetEditSel(int start, int end);
  int     SetItemData(int index, LPARAM data);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT create_struct);
};

////////////////////////////////////////////////////////////////////////////////
// Edit

class Edit : public Window {
public:
  Edit() {}
  Edit(HWND hwnd);
  virtual ~Edit() {}

  void GetRect(LPRECT rect);
  void LimitText(int limit);
  BOOL SetCueBannerText(LPCWSTR text, BOOL draw_focused = FALSE);
  void SetMargins(int left = -1, int right = -1);
  void SetMultiLine(BOOL enabled);
  void SetPasswordChar(UINT ch);
  void SetReadOnly(BOOL read_only);
  void SetRect(LPRECT rect);
  void SetSel(int start, int end);
  BOOL ShowBalloonTip(LPCWSTR text, LPCWSTR title, INT icon);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT create_struct);
};

////////////////////////////////////////////////////////////////////////////////
// Image list

class ImageList {
public:
  ImageList();
  ~ImageList();

  void operator=(const HIMAGELIST image_list);

  int        AddBitmap(HBITMAP bitmap, COLORREF mask);
  BOOL       BeginDrag(int track, int hotspot_x, int hotspot_y);
  BOOL       Create(int cx, int cy);
  VOID       Destroy();
  BOOL       DragEnter(HWND hwnd_lock, int x, int y);
  BOOL       DragLeave(HWND hwnd_lock);
  BOOL       DragMove(int x, int y);
  BOOL       Draw(int index, HDC hdc, int x, int y);
  void       Duplicate(HIMAGELIST image_list);
  VOID       EndDrag();
  HIMAGELIST GetHandle();
  HICON      GetIcon(int index);
  BOOL       GetIconSize(int& cx, int& cy);
  BOOL       Remove(int index);
  VOID       SetHandle(HIMAGELIST image_list);

private:
  HIMAGELIST image_list_;
};

////////////////////////////////////////////////////////////////////////////////
// List view

class ListView : public Window {
public:
  ListView();
  ListView(HWND hwnd);
  virtual ~ListView() {}

  HIMAGELIST CreateDragImage(int item, LPPOINT pt_up_left);
  void       DeleteAllColumns();
  BOOL       DeleteAllItems();
  BOOL       DeleteColumn(int index);
  BOOL       DeleteItem(int item);
  int        EnableGroupView(bool enable);
  BOOL       EnsureVisible(int item);
  BOOL       GetCheckState(UINT index);
  BOOL       GetColumnOrderArray(int count, int* array);
  int        GetCountPerPage();
  HWND       GetHeader();
  int        GetItemCount();
  int        GetItemGroup(int i);
  LPARAM     GetItemParam(int i);
  void       GetItemText(int item, int subitem, LPWSTR output, int max_length = MAX_PATH);
  void       GetItemText(int item, int subitem, std::wstring& output, int max_length = MAX_PATH);
  INT        GetNextItem(int start, UINT flags);
  INT        GetNextItemIndex(int item, int group, LPARAM flags);
  UINT       GetSelectedCount();
  INT        GetSelectionMark();
  int        GetSortColumn();
  int        GetSortOrder();
  int        GetSortType();
  BOOL       GetSubItemRect(int item, int subitem, LPRECT rect);
  int        GetTopIndex();
  DWORD      GetView();
  int        HitTest(bool return_subitem = false);
  int        HitTestEx(LPLVHITTESTINFO lplvhi);
  int        InsertColumn(int index, int width, int width_min, int align, LPCWSTR text);
  int        InsertGroup(int index, LPCWSTR text, bool collapsable = false, bool collapsed = false);
  int        InsertItem(const LVITEM& lvi);
  int        InsertItem(int index, int nGroup, int icon, UINT column_count, PUINT columns, LPCWSTR text, LPARAM lParam);
  BOOL       IsGroupViewEnabled();
  UINT       IsItemVisible(UINT index);
  BOOL       RedrawItems(int first, int last, bool repaint);
  void       RemoveAllGroups();
  void       SelectAllItems(bool selected = true);
  void       SelectItem(int index, bool selected = true);
  BOOL       SetBkImage(HBITMAP bitmap, ULONG flags = LVBKIF_TYPE_WATERMARK, int offset_x = 100, int offset_y = 100);
  void       SetCheckState(int index, BOOL check);
  BOOL       SetColumnOrderArray(int count, int* order_array);
  BOOL       SetColumnWidth(int column, int cx);
  void       SetExtendedStyle(DWORD ex_style);
  int        SetGroupText(int index, LPCWSTR text);
  DWORD      SetHoverTime(DWORD hover_time);
  void       SetImageList(HIMAGELIST image_list, int type = LVSIL_SMALL);
  BOOL       SetItem(int index, int subitem, LPCWSTR text);
  BOOL       SetItemIcon(int index, int icon);
  BOOL       SetItemIcon(int index, int subitem, int icon);
  void       SetItemState(int index, UINT state, UINT mask);
  BOOL       SetTileViewInfo(PLVTILEVIEWINFO tvi);
  BOOL       SetTileViewInfo(int line_count, DWORD flags, RECT* rc_label_margin = nullptr, SIZE* size_tile = nullptr);
  int        SetView(DWORD view);
  void       Sort(int sort_column, int sort_order, int type, PFNLVCOMPARE compare);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT create_struct);

private:
  int sort_column_, sort_order_, sort_type_;
};

////////////////////////////////////////////////////////////////////////////////
// Progress bar

class ProgressBar : public Window {
public:
  ProgressBar() {}
  ProgressBar(HWND hwnd);
  virtual ~ProgressBar() {}

  UINT  GetPosition();
  void  SetMarquee(bool enabled);
  UINT  SetPosition(UINT position);
  DWORD SetRange(UINT min, UINT max);
  UINT  SetState(UINT state);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT create_struct);
};

////////////////////////////////////////////////////////////////////////////////
// Rebar

class Rebar : public Window {
public:
  Rebar() {}
  Rebar(HWND hwnd);
  virtual ~Rebar() {}

  UINT GetBarHeight();
  BOOL InsertBand(LPREBARBANDINFO bar_info);
  BOOL InsertBand(
      HWND hwnd_child,
      UINT cx, UINT cx_header, UINT cx_ideal,
      UINT cx_min_child,
      UINT cy_child, UINT cy_integral,
      UINT cy_max_child, UINT cy_min_child,
      UINT mask, UINT style);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT create_struct);
};

////////////////////////////////////////////////////////////////////////////////
// Rich edit

class RichEdit : public Window {
public:
  RichEdit();
  RichEdit(HWND hwnd);
  virtual ~RichEdit();

  void    GetSel(CHARRANGE* cr);
  std::wstring GetTextRange(CHARRANGE* cr);
  void    HideSelection(BOOL hide);
  BOOL    SetCharFormat(DWORD format, CHARFORMAT* cf);
  DWORD   SetEventMask(DWORD flags);
  void    SetSel(int start, int end);
  void    SetSel(CHARRANGE* cr);
  UINT    SetTextEx(const std::string& text);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT create_struct);

private:
  HMODULE module_msftedit_;
  HMODULE module_riched20_;
};

////////////////////////////////////////////////////////////////////////////////
// Spin

class Spin : public Window {
public:
  Spin() {}
  Spin(HWND hwnd);
  virtual ~Spin() {}

  bool GetPos32(int& value);
  HWND SetBuddy(HWND hwnd);
  int  SetPos32(int position);
  void SetRange32(int lower_limit, int upper_limit);
};

////////////////////////////////////////////////////////////////////////////////
// Status bar

class StatusBar : public Window {
public:
  StatusBar() {}
  StatusBar(HWND hwnd);
  virtual ~StatusBar() {}

  void GetRect(int part, LPRECT rect);
  int  InsertPart(int image, int style, int autosize, int width, LPCWSTR text, LPCWSTR tooltip);
  void SetImageList(HIMAGELIST image_list);
  void SetPartText(int part, LPCWSTR text);
  void SetPartTipText(int part, LPCWSTR tip_text);
  void SetPartWidth(int part, int width);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT create_struct);

private:
  HIMAGELIST image_list_;
  std::vector<int> widths_;
};

////////////////////////////////////////////////////////////////////////////////
// SysLink

class SysLink : public Window {
public:
  SysLink() {}
  SysLink(HWND hwnd);
  virtual ~SysLink() {}

  void SetItemState(int item, UINT states);
};

////////////////////////////////////////////////////////////////////////////////
// Tab

class Tab : public Window {
public:
  Tab() {}
  Tab(HWND hwnd);
  virtual ~Tab() {}

  void   AdjustRect(HWND hwnd, BOOL larger, LPRECT rect);
  int    DeleteAllItems();
  int    DeleteItem(int index);
  int    InsertItem(int index, LPCWSTR text, LPARAM param);
  int    GetCurrentlySelected();
  int    GetItemCount();
  LPARAM GetItemParam(int index);
  int    HitTest();
  int    SetCurrentlySelected(int item);
  int    SetItemText(int item, LPCWSTR text);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT create_struct);
};

////////////////////////////////////////////////////////////////////////////////
// Toolbar

class Toolbar : public Window {
public:
  Toolbar() {}
  Toolbar(HWND hwnd);
  virtual ~Toolbar() {}

  BOOL    EnableButton(int command_id, bool enabled);
  int     GetHeight();
  BOOL    GetButton(int index, TBBUTTON& tbb);
  int     GetButtonCount();
  int     GetButtonIndex(int command_id);
  DWORD   GetButtonSize();
  DWORD   GetButtonStyle(int index);
  LPCWSTR GetButtonTooltip(int index);
  int     GetHotItem();
  DWORD   GetPadding();
  int     HitTest(POINT& point);
  BOOL    InsertButton(int index, int bitmap, int command_id,
                       BYTE state, BYTE style, DWORD_PTR data,
                       LPCWSTR text, LPCWSTR tooltip);
  BOOL    MapAcelerator(UINT character, UINT& command_id);
  BOOL    PressButton(int command_id, BOOL press);
  BOOL    SetButtonImage(int index, int image);
  BOOL    SetButtonText(int index, LPCWSTR text);
  BOOL    SetButtonTooltip(int index, LPCWSTR tooltip);
  int     SetHotItem(int index);
  int     SetHotItem(int index, DWORD flags);
  void    SetImageList(HIMAGELIST image_list, int dx, int dy);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT create_struct);

private:
  std::vector<std::wstring> tooltip_text_;
};

////////////////////////////////////////////////////////////////////////////////
// Tooltip

class Tooltip : public Window {
public:
  Tooltip() {}
  Tooltip(HWND hwnd);
  virtual ~Tooltip() {}

  BOOL AddTip(UINT id, LPCWSTR text, LPCWSTR title, LPRECT rect, bool window_id);
  void DeleteTip(UINT id);
  void NewToolRect(UINT id, LPRECT rect);
  void SetDelayTime(long autopop, long initial, long reshow);
  void SetMaxWidth(long width);
  void UpdateText(UINT id, LPCWSTR text);
  void UpdateTitle(LPCWSTR title);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT create_struct);
};

////////////////////////////////////////////////////////////////////////////////
// Tree view

class TreeView : public Window {
public:
  TreeView() {}
  TreeView(HWND hwnd);
  virtual ~TreeView() {}

  BOOL       DeleteAllItems();
  BOOL       DeleteItem(HTREEITEM item);
  BOOL       Expand(HTREEITEM item, bool expand = true);
  UINT       GetCheckState(HTREEITEM item);
  UINT       GetCount();
  BOOL       GetItem(LPTVITEM item);
  LPARAM     GetItemData(HTREEITEM item);
  HTREEITEM  GetNextItem(HTREEITEM item, UINT flag);
  HTREEITEM  GetSelection();
  HTREEITEM  HitTest(LPTVHITTESTINFO hti, bool get_cursor_pos = false);
  HTREEITEM  InsertItem(LPCWSTR text, int image, LPARAM param,
                        HTREEITEM parent, HTREEITEM insert_after = TVI_LAST);
  BOOL       SelectItem(HTREEITEM item);
  UINT       SetCheckState(HTREEITEM item, BOOL check);
  HIMAGELIST SetImageList(HIMAGELIST image_list, INT image = TVSIL_NORMAL);
  BOOL       SetItem(HTREEITEM item, LPCWSTR text);
  int        SetItemHeight(SHORT cyItem);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT create_struct);
};

}  // namespace win

#endif  // TAIGA_WIN_CTRL_H