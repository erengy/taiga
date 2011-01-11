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

#ifndef DLG_MAIN_H
#define DLG_MAIN_H

#include "../std.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"

enum SearchMode {
  SEARCH_MODE_LIST,
  SEARCH_MODE_MAL,
  SEARCH_MODE_TORRENT,
  SEARCH_MODE_WEB
};

// =============================================================================

class CMainWindow : public CDialog {
public:
  CMainWindow();
  virtual ~CMainWindow() {}

  // Tree control
  class CMainTree : public CTreeView {
  public:
    void RefreshItems();
  private:
    HTREEITEM htItem[7];
  } m_Tree;
  
  // List control
  class CMainList : public CListView {
  public:
    CMainList() { m_bDragging = false; };
    int GetSortType(int column);
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    bool m_bDragging;
    CImageList m_DragImage;
  } m_List;

  // Edit control
  class CEditSearch : public CEdit {
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  } m_EditSearch;

  // Other controls
  CRebar m_Rebar;
  CStatusBar m_Status;
  CTab m_Tab;
  CToolbar m_Toolbar, m_ToolbarSearch;
  CWindow m_CancelSearch;

  // Search bar
  class CSearchBar {
  public:
    CSearchBar() : Index(0), Mode(0) {}
    UINT Index, Mode;
    wstring CueText, URL;
    void SetMode(UINT index, UINT mode, wstring cue_text = L"Search", wstring url = L"");
  } m_SearchBar;
  
  // Message handlers
  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  LRESULT OnButtonCustomDraw(LPARAM lParam);
  BOOL OnClose();
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  void OnDestroy();
  void OnDropFiles(HDROP hDropInfo);
  BOOL OnInitDialog();
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnSize(UINT uMsg, UINT nType, SIZE size);
  void OnTaskbarCallback(UINT uMsg, LPARAM lParam);
  void OnTimer(UINT_PTR nIDEvent);
  LRESULT OnListCustomDraw(LPARAM lParam);
  LRESULT OnListNotify(LPARAM lParam);
  LRESULT OnTabNotify(LPARAM lParam);
  LRESULT OnToolbarNotify(LPARAM lParam);
  LRESULT OnTreeNotify(LPARAM lParam);
  BOOL PreTranslateMessage(MSG* pMsg);

  // Other functions
  void ChangeStatus(wstring str = L"");
  int GetListIndex(int anime_index);
  void RefreshList(int index = -1);
  void RefreshMenubar(int index = -1, bool show = true);
  void RefreshTabs(int index = -1, bool redraw = true);
  void UpdateTip();
};

extern CMainWindow MainWindow;

#endif // DLG_MAIN_H