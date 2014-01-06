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

#ifndef TAIGA_UI_DLG_MAIN_H
#define TAIGA_UI_DLG_MAIN_H

#include "library/anime_filter.h"
#include "win/ctrl/win_ctrl.h"
#include "win/win_dialog.h"
#include "win/win_gdi.h"

#define WM_TAIGA_SHOWMENU WM_USER + 1337

namespace ui {

enum MainToolbarButtons {
  kToolbarButtonSync = 200,
  kToolbarButtonMal = 201,
  kToolbarButtonHerro = 202,
  kToolbarButtonFolders = 204,
  kToolbarButtonTools = 205,
  kToolbarButtonSettings = 207,
  kToolbarButtonDebug = 209
};

enum SearchMode {
  kSearchModeNone,
  kSearchModeService,
  kSearchModeFeed
};

enum SidebarItems {
  kSidebarItemNowPlaying = 0,
  kSidebarItemAnimeList = 2,
  kSidebarItemHistory = 3,
  kSidebarItemStats = 4,
  kSidebarItemSearch = 6,
  kSidebarItemSeasons = 7,
  kSidebarItemFeeds = 8
};

class MainDialog : public win::Dialog {
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
  void ChangeStatus(std::wstring str = L"");
  void EnableInput(bool enable = true);
  void UpdateControlPositions(const SIZE* size = nullptr);
  void UpdateStatusTimer();
  void UpdateTip();
  void UpdateTitle();

  class Navigation {
  public:
    Navigation() : current_page_(-1), index_(-1), parent(nullptr) {}
    int GetCurrentPage();
    void SetCurrentPage(int page, bool reorder = true);
    void GoBack();
    void GoForward();
    void Refresh(bool add_to_history);
    MainDialog* parent;
  private:
    int current_page_;
    int index_;
    std::vector<int> items_;
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
    win::Toolbar* toolbar;
  } toolbar_wm;

public:
  // TreeView control
  class MainTree : public win::TreeView {
  public:
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL IsVisible();
    void RefreshHistoryCounter();
    std::vector<HTREEITEM> hti;
  } treeview;

  // Edit control
  class EditSearch : public win::Edit {
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  } edit;

  // Cancel button
  class CancelButton : public win::Window {
  public:
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnCustomDraw(LPARAM lParam);
  } cancel_button;

  // Other controls
  win::Rebar rebar;
  win::StatusBar statusbar;
  win::Toolbar toolbar_menu, toolbar_main, toolbar_search;

  // Search bar
  class SearchBar {
  public:
    SearchBar() : mode(kSearchModeNone), parent(nullptr) {}
    int mode;
    MainDialog* parent;
    anime::Filters filters;
  } search_bar;

private:
  win::Rect rect_content_, rect_sidebar_;
};

extern MainDialog DlgMain;

}  // namespace ui

#endif  // TAIGA_UI_DLG_MAIN_H