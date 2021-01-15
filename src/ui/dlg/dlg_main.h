/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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

#pragma once

#include <map>
#include <vector>

#include <windows/win/common_controls.h>
#include <windows/win/dialog.h>
#include <windows/win/gdi.h>
#include <windows/win/snappable.h>

#include "media/anime_filter.h"

constexpr unsigned int WM_TAIGA_SHOWMENU = WM_USER + 1337;

namespace ui {

enum MainToolbarButtons {
  kToolbarButtonSync = 200,
  kToolbarButtonFolders = 202,
  kToolbarButtonTools = 203,
  kToolbarButtonSettings = 205,
  kToolbarButtonDebug = 207
};

enum class SearchMode {
  None,
  Service,
  Feed,
};

enum SidebarItems {
  kSidebarItemNowPlaying,
  kSidebarItemSeparator1,
  kSidebarItemAnimeList,
  kSidebarItemHistory,
  kSidebarItemStats,
  kSidebarItemSeparator2,
  kSidebarItemSearch,
  kSidebarItemSeasons,
  kSidebarItemFeeds
};

class MainDialog : public win::Dialog, public win::Snappable {
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
  LRESULT OnStatusbarNotify(LPARAM lParam);
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
    void SetCurrentPage(int page, bool add_to_history = true);
    void GoBack();
    void GoForward();
    void Refresh(bool add_to_history);
    void RefreshSearchText(int previous_page);
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
    bool IsSeparator(int page);
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

  // Statusbar
  class StatusBar : public win::StatusBar {
  public:
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  } statusbar;

  // Other controls
  win::Rebar rebar;
  win::Toolbar toolbar_menu, toolbar_main, toolbar_search;

  // Search bar
  class SearchBar {
  public:
    SearchBar() : mode(SearchMode::None), parent(nullptr) {}
    SearchMode mode;
    MainDialog* parent;
    anime::Filters filters;
    std::map<int, std::wstring> text;
  } search_bar;

private:
  win::Rect rect_content_, rect_sidebar_;
};

extern MainDialog DlgMain;

}  // namespace ui
