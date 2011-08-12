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

#ifndef DLG_SEASON_H
#define DLG_SEASON_H

#include "../std.h"
#include "../gfx.h"
#include "../http.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"

// =============================================================================

/* Season browser */

enum {
  SEASONBROWSER_GROUPBY_AIRINGSTATUS,
  SEASONBROWSER_GROUPBY_LISTSTATUS,
  SEASONBROWSER_GROUPBY_TYPE
};

enum {
  SEASONBROWSER_SORTBY_AIRINGDATE,
  SEASONBROWSER_SORTBY_EPISODES,
  SEASONBROWSER_SORTBY_POPULARITY,
  SEASONBROWSER_SORTBY_SCORE,
  SEASONBROWSER_SORTBY_TITLE
};

class CSeasonWindow : public CDialog {
public:
  CSeasonWindow();
  virtual ~CSeasonWindow() {}

  LRESULT OnButtonCustomDraw(LPARAM lParam);
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnDestroy();
  BOOL OnInitDialog();
  LRESULT OnListCustomDraw(LPARAM lParam);
  LRESULT OnListNotify(LPARAM lParam);
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnSize(UINT uMsg, UINT nType, SIZE size);
  LRESULT OnToolbarNotify(LPARAM lParam);
  BOOL PreTranslateMessage(MSG* pMsg);

public:
  void RefreshData(bool connect = true);
  void RefreshList();
  void RefreshStatus();
  void RefreshToolbar();

public:
  class CEditFilter : public CEdit {
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  } m_EditFilter;

  CListView m_List;
  CRebar m_Rebar;
  CStatusBar m_Status;
  CToolbar m_Toolbar, m_ToolbarFilter;
  CWindow m_CancelFilter;

public:
  vector<CImage> Images;
  vector<CHTTPClient> ImageClients, InfoClients;
  int GroupBy, SortBy;
};

extern CSeasonWindow SeasonWindow;

#endif // DLG_SEASON_H