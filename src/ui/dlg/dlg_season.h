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

#include <windows/win/common_controls.h>
#include <windows/win/dialog.h>

namespace ui {

enum SeasonGroupBy {
  kSeasonGroupByAiringStatus,
  kSeasonGroupByListStatus,
  kSeasonGroupByType
};

enum SeasonSortBy {
  kSeasonSortByAiringDate,
  kSeasonSortByEpisodes,
  kSeasonSortByPopularity,
  kSeasonSortByScore,
  kSeasonSortByTitle
};

enum SeasonViewAs {
  kSeasonViewAsImages,
  kSeasonViewAsTiles
};

class SeasonDialog : public win::Dialog {
public:
  SeasonDialog();
  ~SeasonDialog() {}

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  void OnContextMenu(HWND hwnd, POINT pt);
  BOOL OnDestroy();
  BOOL OnInitDialog();
  LRESULT OnListCustomDraw(LPARAM lParam);
  LRESULT OnListNotify(LPARAM lParam);
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnSize(UINT uMsg, UINT nType, SIZE size);
  LRESULT OnToolbarNotify(LPARAM lParam);

  void EnableInput(bool enable = true);
  void RefreshData(const std::vector<int>& anime_ids);
  void RefreshList(bool redraw_only = false);
  void RefreshStatus();
  void RefreshToolbar();
  void SetViewMode(int mode);

private:
  int GetLineCount() const;

  class ListView : public win::ListView {
  public:
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  } list_;

  win::Window cancel_button_;
  win::Rebar rebar_;
  win::Toolbar toolbar_;
  win::Tooltip tooltips_;

  int hot_item_;
};

extern SeasonDialog DlgSeason;

}  // namespace ui
