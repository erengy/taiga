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

enum SearchMode {
  SEARCH_MODE_MAL,
  SEARCH_MODE_TORRENT,
  SEARCH_MODE_WEB
};

// =============================================================================

class MainDialog : public win32::Dialog {
public:
  MainDialog();
  virtual ~MainDialog() {}

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  LRESULT OnButtonCustomDraw(LPARAM lParam);
  BOOL OnClose();
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnDestroy();
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

public:
  void ChangeStatus(wstring str = L"");
  void EnableInput(bool enable = true);
  int GetListIndex(int anime_id);
  void RefreshList(int index = -1);
  void RefreshTabs(int index = -1, bool redraw = true);
  void UpdateStatusTimer();
  void UpdateTip();

private:
  void CreateDialogControls();
  void InitWindowPosition();

public:
  // Tree-view control
  class CMainTree : public win32::TreeView {
  public:
    void RefreshItems();
  private:
    HTREEITEM htItem[7];
  } treeview;
  
  // List-view control
  class ListView : public win32::ListView {
  public:
    ListView() { dragging = false; };
    int GetSortType(int column);
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    bool dragging;
    win32::ImageList drag_image;
    MainDialog* parent;
  } listview;

  // Edit control
  class EditSearch : public win32::Edit {
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  } edit;

  // Other controls
  win32::Window cancel_button;
  win32::Rebar rebar;
  win32::StatusBar statusbar;
  win32::Tab tab;
  win32::Toolbar toolbar_menu, toolbar_main, toolbar_search;

  // Search bar
  class SearchBar {
  public:
    SearchBar() : filter_list(true), index(2), mode(SEARCH_MODE_MAL) {}
    bool filter_list;
    UINT index, mode;
    wstring cue_text, url;
    MainDialog* parent;
    void SetMode(UINT index, UINT mode, wstring cue_text = L"Search", wstring url = L"");
  } search_bar;
};

extern class MainDialog MainDialog;

#endif // DLG_MAIN_H