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

#ifndef WIN_CONTROL_H
#define WIN_CONTROL_H

#include <windows.h>
#include <commctrl.h>
#include <richedit.h>

#include "win_main.h"
#include "win_window.h"

namespace win32 {

// =============================================================================

/* ComboBox */

class ComboBox : public Window {
public:
  ComboBox() {}
  ComboBox(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~ComboBox() {}

  int     AddItem(LPCWSTR lpsz, LPARAM data);
  int     AddString(LPCWSTR lpsz);
  int     DeleteString(int index);
  int     GetCount();
  LRESULT GetItemData(int index);
  int     GetCurSel();
  void    ResetContent();
  int     SetCurSel(int index);
  BOOL    SetEditSel(int ichStart, int ichEnd);
  int     SetItemData(int index, LPARAM data);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
};

// =============================================================================

/* Edit */

class Edit : public Window {
public:
  Edit() {}
  Edit(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~Edit() {}

  void GetRect(LPRECT lprc);
  void LimitText(int cchMax);
  BOOL SetCueBannerText(LPCWSTR lpcwText, BOOL fDrawFocused = FALSE);
  void SetMargins(int iLeft = -1, int iRight = -1);
  void SetMultiLine(BOOL bEnabled);
  void SetPasswordChar(UINT ch);
  void SetReadOnly(BOOL bReadOnly);
  void SetRect(LPRECT lprc);
  void SetSel(int ichStart, int ichEnd);
  BOOL ShowBalloonTip(LPCWSTR pszText, LPCWSTR pszTitle, INT ttiIcon);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
};

// =============================================================================

/* Image list */

class ImageList {
public:
  ImageList() { m_hImageList = NULL; }
  ~ImageList() { Destroy(); }
  void operator = (const HIMAGELIST hImageList) {
    SetHandle(hImageList);
  }
  
  int        AddBitmap(HBITMAP hBitmap, COLORREF crMask);
  BOOL       BeginDrag(int iTrack, int dxHotspot, int dyHotspot);
  BOOL       Create(int cx, int cy);
  VOID       Destroy();
  BOOL       DragEnter(HWND hwndLock, int x, int y);
  BOOL       DragLeave(HWND hwndLock);
  BOOL       DragMove(int x, int y);
  BOOL       Draw(int nIndex, HDC hdcDest, int x, int y);
  VOID       EndDrag();
  HIMAGELIST GetHandle();
  HICON      GetIcon(int nIndex);
  BOOL       Remove(int index);
  VOID       SetHandle(HIMAGELIST hImageList);

private:
  HIMAGELIST m_hImageList;
};

// =============================================================================

/* List view */

class ListView : public Window {
public:
  ListView();
  ListView(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~ListView() {}
  
  HIMAGELIST CreateDragImage(int iItem, LPPOINT lpptUpLeft);
  BOOL       DeleteAllItems();
  BOOL       DeleteItem(int iItem);
  int        EnableGroupView(bool bValue);
  BOOL       EnsureVisible(int i);
  BOOL       GetCheckState(UINT iIndex);
  HWND       GetHeader();
  int        GetItemCount();
  LPARAM     GetItemParam(int i);
  void       GetItemText(int iItem, int iSubItem, LPWSTR pszText, int cchTextMax = MAX_PATH);
  void       GetItemText(int iItem, int iSubItem, wstring& str, int cchTextMax = MAX_PATH);
  INT        GetNextItem(int iStart, UINT flags);
  INT        GetNextItemIndex(int iItem, int iGroup, LPARAM flags);
  UINT       GetSelectedCount();
  INT        GetSelectionMark();
  int        GetSortColumn();
  int        GetSortOrder();
  int        GetSortType();
  BOOL       GetSubItemRect(int iItem, int iSubItem, LPRECT lpRect);
  DWORD      GetView();
  int        HitTest(bool return_subitem = false);
  int        HitTestEx(LPLVHITTESTINFO lplvhi);
  int        InsertColumn(int nIndex, int nWidth, int nWidthMin, int nAlign, LPCWSTR szText);
  int        InsertGroup(int nIndex, LPCWSTR szText, bool bCollapsable = false, bool bCollapsed = false);
  int        InsertItem(const LVITEM& lvi);
  int        InsertItem(int nIndex, int nGroup, int nIcon, UINT cColumns, PUINT puColumns, LPCWSTR pszText, LPARAM lParam);
  BOOL       IsGroupViewEnabled();
  BOOL       RedrawItems(int iFirst, int iLast, bool repaint);
  void       RemoveAllGroups();
  BOOL       SetBkImage(HBITMAP hbmp, ULONG ulFlags = LVBKIF_TYPE_WATERMARK, int xOffset = 100, int yOffset = 100);
  void       SetCheckState(int iIndex, BOOL fCheck);
  BOOL       SetColumnWidth(int iCol, int cx);
  void       SetExtendedStyle(DWORD dwExStyle);
  int        SetGroupText(int nIndex, LPCWSTR szText);
  void       SetImageList(HIMAGELIST hImageList, int iImageList = LVSIL_SMALL);
  BOOL       SetItem(int nIndex, int nSubItem, LPCWSTR szText);
  BOOL       SetItemIcon(int nIndex, int nIcon);
  void       SetSelectedItem(int iIndex);
  BOOL       SetTileViewInfo(PLVTILEVIEWINFO plvtvinfo);
  BOOL       SetTileViewInfo(int cLines, DWORD dwFlags, RECT* rcLabelMargin = NULL, SIZE* sizeTile = NULL);
  int        SetView(DWORD iView);
  void       Sort(int iColumn, int iOrder, int iType, PFNLVCOMPARE pfnCompare);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);

private:
  int m_iSortColumn, m_iSortOrder, m_iSortType;
};

// =============================================================================

/* Progress bar */

class ProgressBar : public Window {
public:
  ProgressBar() {}
  ProgressBar(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~ProgressBar() {}

  UINT  GetPosition();
  void  SetMarquee(bool enabled);
  UINT  SetPosition(UINT position);
  DWORD SetRange(UINT min, UINT max);
  UINT  SetState(UINT state);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
};

// =============================================================================

/* Rebar */

class Rebar : public Window {
public:
  Rebar() {}
  Rebar(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~Rebar() {}

  UINT GetBarHeight();
  BOOL InsertBand(LPREBARBANDINFO lpBarInfo);
  BOOL InsertBand(HWND hwndChild, UINT cx, UINT cxHeader, UINT cxIdeal, UINT cxMinChild, 
                  UINT cyChild, UINT cyIntegral, UINT cyMaxChild, UINT cyMinChild, 
                  UINT fMask, UINT fStyle);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
};

// =============================================================================

/* Rich edit */

class RichEdit : public Window {
public:
  RichEdit();
  RichEdit(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~RichEdit();

  void    GetSel(CHARRANGE* cr);
  wstring GetTextRange(CHARRANGE* cr);
  void    HideSelection(BOOL bHide);
  BOOL    SetCharFormat(DWORD dwFormat, CHARFORMAT* cf);
  DWORD   SetEventMask(DWORD dwFlags);
  void    SetSel(int ichStart, int ichEnd);
  void    SetSel(CHARRANGE* cr);
  UINT    SetTextEx(const string& str);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);

private:
  HMODULE m_hInstRichEdit;
};

// =============================================================================

/* Spin */

class Spin : public Window {
public:
  Spin() {}
  Spin(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~Spin() {}

  bool GetPos32(int& value);
  HWND SetBuddy(HWND hwnd);
  int  SetPos32(int position);
  void SetRange32(int lower_limit, int upper_limit);
};

// =============================================================================

/* Status bar */

class StatusBar : public Window {
public:
  StatusBar() {}
  StatusBar(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~StatusBar() {}

  int  InsertPart(int iImage, int iStyle, int iAutosize, int iWidth, LPCWSTR lpText, LPCWSTR lpTooltip);
  void SetImageList(HIMAGELIST hImageList);
  void SetPartText(int iPart, LPCWSTR lpText);
  void SetPartTipText(int iPart, LPCWSTR lpTipText);
  void SetPartWidth(int iPart, int iWidth);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);

private:
  HIMAGELIST  m_hImageList;
  vector<int> m_iWidth;
};

// =============================================================================

/* Tab */

class Tab : public Window {
public:
  Tab() {}
  Tab(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~Tab() {}

  void   AdjustRect(HWND hWindow, BOOL fLarger, LPRECT lpRect);
  int    Clear();
  int    DeleteItem(int nIndex);
  int    InsertItem(int nIndex, LPCWSTR szText, LPARAM lParam);
  int    GetCurrentlySelected();
  int    GetItemCount();
  LPARAM GetItemParam(int nIndex);
  int    HitTest();
  int    SetCurrentlySelected(int iItem);
  int    SetItemText(int iItem, LPCWSTR szText);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
};

// =============================================================================

/* Toolbar */

class Toolbar : public Window {
public:
  Toolbar() {}
  Toolbar(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~Toolbar() {}

  BOOL    EnableButton(int idCommand, bool bEnabled);
  int     GetHeight();
  BOOL    GetButton(int nIndex, TBBUTTON& tbb);
  int     GetButtonCount();
  DWORD   GetButtonSize();
  DWORD   GetButtonStyle(int nIndex);
  LPCWSTR GetButtonTooltip(int nIndex);
  DWORD   GetPadding();
  int     HitTest(POINT& pt);
  BOOL    InsertButton(int iIndex, int iBitmap, int idCommand, 
                       bool bEnabled, BYTE fsStyle, DWORD_PTR dwData, 
                       LPCWSTR lpText, LPCWSTR lpTooltip);
  BOOL    PressButton(int idCommand, BOOL bPress);
  BOOL    SetButtonImage(int nIndex, int iImage);
  BOOL    SetButtonText(int nIndex, LPCWSTR lpText);
  BOOL    SetButtonTooltip(int nIndex, LPCWSTR lpTooltip);
  void    SetImageList(HIMAGELIST hImageList, int dxBitmap, int dyBitmap);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);

private:
  vector<LPCWSTR> m_TooltipText;
};

// =============================================================================

/* Tooltip */

class Tooltip : public Window {
public:
  Tooltip() {}
  Tooltip(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~Tooltip() {}
  
  BOOL AddTip(UINT uID, LPCWSTR lpText, LPCWSTR lpTitle, LPRECT rcArea, bool bWindowID);
  BOOL DeleteTip(UINT uID);
  void SetDelayTime(long lAutopop, long lInitial, long lReshow);
  void SetMaxWidth(long lWidth);
  void UpdateText(UINT uID, LPCWSTR lpText);
  void UpdateTitle(LPCWSTR lpTitle);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
};

// =============================================================================

/* Tree view */

class TreeView : public Window {
public:
  TreeView() {}
  TreeView(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~TreeView() {}

  BOOL       DeleteAllItems();
  BOOL       DeleteItem(HTREEITEM hitem);
  BOOL       Expand(HTREEITEM hItem, bool bExpand = true);
  UINT       GetCheckState(HTREEITEM hItem);
  UINT       GetCount();
  BOOL       GetItem(LPTVITEM pItem);
  LPARAM     GetItemData(HTREEITEM hItem);
  HTREEITEM  GetSelection();
  HTREEITEM  HitTest(LPTVHITTESTINFO lpht, bool bGetCursorPos = false);
  HTREEITEM  InsertItem(LPCWSTR pszText, int iImage, LPARAM lParam, 
                        HTREEITEM htiParent, HTREEITEM hInsertAfter = TVI_LAST);
  BOOL       SelectItem(HTREEITEM hItem);
  UINT       SetCheckState(HTREEITEM hItem, BOOL fCheck);
  HIMAGELIST SetImageList(HIMAGELIST himl, INT iImage = TVSIL_NORMAL);
  BOOL       SetItem(HTREEITEM hItem, LPCWSTR pszText);
  int        SetItemHeight(SHORT cyItem);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
};

} // namespace win32

#endif // WIN_CONTROL_H