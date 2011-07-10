/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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

// =============================================================================

/* ComboBox */

class CComboBox : public CWindow {
public:
  CComboBox() {}
  CComboBox(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~CComboBox() {}

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

class CEdit : public CWindow {
public:
  CEdit() {}
  CEdit(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~CEdit() {}

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

class CImageList {
public:
  CImageList() { m_hImageList = NULL; }
  ~CImageList() { Destroy(); }
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

class CListView : public CWindow {
public:
  CListView();
  CListView(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~CListView() {}
  
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

class CProgressBar : public CWindow {
public:
  CProgressBar() {}
  CProgressBar(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~CProgressBar() {}

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

class CRebar : public CWindow {
public:
  CRebar() {}
  CRebar(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~CRebar() {}

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

class CRichEdit : public CWindow {
public:
  CRichEdit();
  CRichEdit(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~CRichEdit();

  void  GetSel(CHARRANGE* cr);
  void  HideSelection(BOOL bHide);
  BOOL  SetCharFormat(DWORD dwFormat, CHARFORMAT* cf);
  DWORD SetEventMask(DWORD dwFlags);
  void  SetSel(int ichStart, int ichEnd);
  void  SetSel(CHARRANGE* cr);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);

private:
  HMODULE m_hInstRichEdit;
};

// =============================================================================

/* Spin */

class CSpin : public CWindow {
public:
  CSpin() {}
  CSpin(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~CSpin() {}

  HWND SetBuddy(HWND hwnd);
  int  SetPos32(int position);
  void SetRange32(int lower_limit, int upper_limit);
};

// =============================================================================

/* Status bar */

class CStatusBar : public CWindow {
public:
  CStatusBar() {}
  CStatusBar(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~CStatusBar() {}

  int  InsertPart(int iImage, int iStyle, int iAutosize, int iWidth, LPCWSTR lpText, LPCWSTR lpTooltip);
  void SetImageList(HIMAGELIST hImageList);
  void SetPanelText(int iPanel, LPCWSTR lpText);
  void SetPanelTipText(int iPanel, LPCWSTR lpTipText);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);

private:
  HIMAGELIST  m_hImageList;
  vector<int> m_iWidth;
};

// =============================================================================

/* Tab */

class CTab : public CWindow {
public:
  CTab() {}
  CTab(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~CTab() {}

  void   AdjustRect(BOOL fLarger, LPRECT lpRect);
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

class CToolbar : public CWindow {
public:
  CToolbar() {}
  CToolbar(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~CToolbar() {}

  BOOL    EnableButton(int nIndex, bool bEnabled);
  int     GetHeight();
  DWORD   GetButtonSize();
  LPCWSTR GetButtonTooltip(int nIndex);
  DWORD   GetPadding();
  BOOL    InsertButton(int iIndex, int iBitmap, int idCommand, 
                       bool bEnabled, BYTE fsStyle, DWORD_PTR dwData, 
                       LPCWSTR lpText, LPCWSTR lpTooltip);
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

class CTooltip : public CWindow {
public:
  CTooltip() {}
  CTooltip(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~CTooltip() {}
  
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

class CTreeView : public CWindow {
public:
  CTreeView() {}
  CTreeView(HWND hWnd) { SetWindowHandle(hWnd); }
  virtual ~CTreeView() {}

  BOOL      DeleteAllItems();
  BOOL      DeleteItem(HTREEITEM hitem);
  BOOL      Expand(HTREEITEM hItem, bool bExpand = true);
  UINT      GetCheckState(HTREEITEM hItem);
  UINT      GetCount();
  BOOL      GetItem(LPTVITEM pItem);
  LPARAM    GetItemData(HTREEITEM hItem);
  HTREEITEM GetSelection();
  HTREEITEM InsertItem(LPCWSTR pszText, LPARAM lParam, HTREEITEM htiParent, HTREEITEM hInsertAfter = TVI_LAST);
  BOOL      SelectItem(HTREEITEM hItem);
  UINT      SetCheckState(HTREEITEM hItem, BOOL fCheck);
  int       SetItemHeight(SHORT cyItem);

protected:
  virtual void PreCreate(CREATESTRUCT &cs);
  virtual void OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
};

#endif // WIN_CONTROL_H