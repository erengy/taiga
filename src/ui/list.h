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

#include <vector>
#include <windows.h>

namespace win {
class ListView;
}

namespace ui {

enum ListSortOrder {
  kListSortAscending = 1,
  kListSortDescending = -1,
};

enum ListSortType {
  kListSortDefault,
  kListSortFileSize,
  kListSortNumber,
  kListSortDateStart,
  kListSortEpisodeCount,
  kListSortEpisodeRange,
  kListSortMyDateCompleted,
  kListSortMyDateStart,
  kListSortLastUpdated,
  kListSortPopularity,
  kListSortProgress,
  kListSortRfc822DateTime,
  kListSortMyScore,
  kListSortScore,
  kListSortSeason,
  kListSortStatus,
  kListSortTitle,
};

int CALLBACK ListViewCompareProc(LPARAM lParam1, LPARAM lParam2,
                                 LPARAM lParamSort);

int CALLBACK AnimeListCompareProc(LPARAM lParam1, LPARAM lParam2,
                                  LPARAM lParamSort);

int GetAnimeIdFromSelectedListItem(win::ListView& listview);
std::vector<int> GetAnimeIdsFromSelectedListItems(win::ListView& listview);
LPARAM GetParamFromSelectedListItem(win::ListView& listview);
std::vector<LPARAM> GetParamsFromSelectedListItems(win::ListView& listview);

void GetPopupMenuPositionForSelectedListItem(win::ListView& listview, POINT& pt);
bool HitTestListHeader(win::ListView& listview, POINT pt);

}  // namespace ui
