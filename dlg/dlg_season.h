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

#ifndef DLG_SEASON_H
#define DLG_SEASON_H

#include "../std.h"

#include "../gfx.h"
#include "../http.h"

#include "../win32/win_control.h"
#include "../win32/win_dialog.h"

// =============================================================================

enum SeasonGroupBy {
  SEASON_GROUPBY_AIRINGSTATUS,
  SEASON_GROUPBY_LISTSTATUS,
  SEASON_GROUPBY_TYPE
};

enum SeasonSortBy {
  SEASON_SORTBY_AIRINGDATE,
  SEASON_SORTBY_EPISODES,
  SEASON_SORTBY_POPULARITY,
  SEASON_SORTBY_SCORE,
  SEASON_SORTBY_TITLE
};

class SeasonDialog : public win32::Dialog {
public:
  SeasonDialog();
  virtual ~SeasonDialog() {}

  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnDestroy();
  BOOL OnInitDialog();
  LRESULT OnListCustomDraw(LPARAM lParam);
  LRESULT OnListNotify(LPARAM lParam);
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnSize(UINT uMsg, UINT nType, SIZE size);
  LRESULT OnToolbarNotify(LPARAM lParam);

public:
  void RefreshData(bool connect = true, int anime_id = 0);
  void RefreshList(bool redraw_only = false);
  void RefreshStatus();
  void RefreshToolbar();

public:
  wstring filter_text;
  int group_by, sort_by;
  vector<Image> images;

private:
  vector<class HttpClient> image_clients_, info_clients_;

private:
  win32::Window cancel_button_;
  win32::ListView list_;
  win32::Rebar rebar_;
  win32::Toolbar toolbar_;
};

extern class SeasonDialog SeasonDialog;

#endif // DLG_SEASON_H