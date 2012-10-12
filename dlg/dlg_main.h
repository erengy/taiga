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

#ifndef DLG_MAIN_H
#define DLG_MAIN_H

#include "../std.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"
#include "../win32/win_gdi.h"

#define WM_TAIGA_SHOWMENU WM_USER + 1337

enum SearchMode {
  SEARCH_MODE_MAL,
  SEARCH_MODE_TORRENT,
  SEARCH_MODE_WEB
};

enum SidebarItems {
  SIDEBAR_ITEM_DASHBOARD = 0,
  SIDEBAR_ITEM_NOWPLAYING = 1,
  SIDEBAR_ITEM_ANIMELIST = 3,
  SIDEBAR_ITEM_MANGALIST = 4,
  SIDEBAR_ITEM_HISTORY = 6,
  SIDEBAR_ITEM_STATS = 7,
  SIDEBAR_ITEM_SEARCH = 9,
  SIDEBAR_ITEM_SEASONS = 10,
  SIDEBAR_ITEM_FEEDS = 12
};

// =============================================================================

class MainDialog : public win32::Dialog {
public:
  MainDialog();
  virtual ~MainDialog() {}

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnClose();
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnDestroy();
  void OnDropFiles(HDROP hDropInfo);
  BOOL OnInitDialog();
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnPaint(HDC hdc, LPPAINTSTRUCT lpps);
  void OnSize(UINT uMsg, UINT nType, SIZE size);
  void OnTaskbarCallback(UINT uMsg, LPARAM lParam);
  void OnTimer(UINT_PTR nIDEvent);
  LRESULT OnToolbarNotify(LPARAM lParam);
  LRESULT OnTreeNotify(LPARAM lParam);
  BOOL PreTranslateMessage(MSG* pMsg);

public:
  void ChangeStatus(wstring str = L"");
  void EnableInput(bool enable = true);
  int GetCurrentPage();
  void SetCurrentPage(int page);
  void UpdateControlPositions(const SIZE* size = nullptr);
  void UpdateStatusTimer();
  void UpdateTip();

private:
  void CreateDialogControls();
  void InitWindowPosition();

  class ToolbarWithMenu {
  public:
    ToolbarWithMenu() : button_index(-1), hook(nullptr), toolbar(nullptr) {}
    static LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam);
    void ShowMenu();
    int button_index;
    HHOOK hook;
    win32::Toolbar* toolbar;
  } toolbar_wm;

public:
  // Tree-view control
  class CMainTree : public win32::TreeView {
  public:
    void RefreshItems();
    vector<HTREEITEM> hti;
  } treeview;

  // Edit control
  class EditSearch : public win32::Edit {
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  } edit;

  // Cancel button
  class CancelButton : public win32::Window {
  public:
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnCustomDraw(LPARAM lParam);
  } cancel_button;

  // Other controls
  win32::Rebar rebar;
  win32::StatusBar statusbar;
  win32::Toolbar toolbar_menu, toolbar_main, toolbar_search;

  // Search bar
  class SearchBar {
  public:
    SearchBar() : filter_content(true), index(2), mode(SEARCH_MODE_MAL) {}
    bool filter_content;
    UINT index, mode;
    wstring cue_text, url;
    MainDialog* parent;
    void SetMode(UINT index, UINT mode, wstring cue_text = L"Search", wstring url = L"");
  } search_bar;

private:
  int current_page_;
  win32::Rect rect_content_, rect_sidebar_;
};

extern class MainDialog MainDialog;

#endif // DLG_MAIN_H