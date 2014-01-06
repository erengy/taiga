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

#ifndef TAIGA_UI_DLG_SEASON_H
#define TAIGA_UI_DLG_SEASON_H

#include "win/ctrl/win_ctrl.h"
#include "win/win_dialog.h"

namespace ui {

class SeasonDialog : public win::Dialog {
public:
  SeasonDialog();
  ~SeasonDialog() {}

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnDestroy();
  BOOL OnInitDialog();
  LRESULT OnListCustomDraw(LPARAM lParam);
  LRESULT OnListNotify(LPARAM lParam);
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnSize(UINT uMsg, UINT nType, SIZE size);
  LRESULT OnToolbarNotify(LPARAM lParam);

  void RefreshData(int anime_id = 0);
  void RefreshList(bool redraw_only = false);
  void RefreshStatus();
  void RefreshToolbar();
  void SetViewMode(int mode);

  int group_by;
  int sort_by;
  int view_as;

private:
  win::Window cancel_button_;
  class ListView : public win::ListView {
  public:
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  } list_;
  win::Rebar rebar_;
  win::Toolbar toolbar_;
};

extern SeasonDialog DlgSeason;

}  // namespace ui

#endif  // TAIGA_UI_DLG_SEASON_H