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

#include "../anime_filter.h"

#include "../win32/win_control.h"
#include "../win32/win_dialog.h"
#include "../win32/win_gdi.h"

#define WM_TAIGA_SHOWMENU WM_USER + 1337

enum MainToolbarButtons {
  TOOLBAR_BUTTON_SYNCHRONIZE = 200,
  TOOLBAR_BUTTON_MAL = 201,
  TOOLBAR_BUTTON_FOLDERS = 203,
  TOOLBAR_BUTTON_TOOLS = 204,
  TOOLBAR_BUTTON_SETTINGS = 206,
  TOOLBAR_BUTTON_ABOUT = 208
};

enum SearchMode {
  SEARCH_MODE_NONE,
  SEARCH_MODE_MAL,
  SEARCH_MODE_FEED
};

enum SidebarItems {
  SIDEBAR_ITEM_NOWPLAYING = 0,
  SIDEBAR_ITEM_ANIMELIST = 2,
  SIDEBAR_ITEM_HISTORY = 3,
  SIDEBAR_ITEM_STATS = 4,
  SIDEBAR_ITEM_SEARCH = 6,
  SIDEBAR_ITEM_SEASONS = 7,
  SIDEBAR_ITEM_FEEDS = 8
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
  void UpdateControlPositions(const SIZE* size = nullptr);
  void UpdateStatusTimer();
  void UpdateTip();
  void UpdateTitle();

  class Navigation {
  public:
    Navigation() : current_page_(-1), index_(-1) {}
    int GetCurrentPage();
    void SetCurrentPage(int page, bool reorder = true);
    void GoBack();
    void GoForward();
    void Refresh(bool add_to_history);
    MainDialog* parent;
  private:
    int current_page_;
    int index_;
    vector<int> items_;
  } navigation;

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
  // TreeView control
  class MainTree : public win32::TreeView {
  public:
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL IsVisible();
    void RefreshHistoryCounter();
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
    SearchBar() : mode(SEARCH_MODE_NONE) {}
    int mode;
    MainDialog* parent;
    anime::Filters filters;
  } search_bar;

private:
  win32::Rect rect_content_, rect_sidebar_;
};

extern class MainDialog MainDialog;

#endif // DLG_MAIN_H