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

#ifndef TAIGA_UI_DLG_ANIME_LIST_H
#define TAIGA_UI_DLG_ANIME_LIST_H

#include "win/ctrl/win_ctrl.h"
#include "win/win_dialog.h"
#include "win/win_gdi.h"

namespace anime {
class Item;
}

namespace ui {

enum AnimeListAction {
  kAnimeListActionNothing,
  kAnimeListActionEdit,
  kAnimeListActionOpenFolder,
  kAnimeListActionPlayNext,
  kAnimeListActionInfo,
  kAnimeListActionPage,
};

enum AnimeListColumn {
  kColumnUnknown,
  kColumnAnimeRating,
  kColumnAnimeSeason,
  kColumnAnimeStatus,
  kColumnAnimeTitle,
  kColumnAnimeType,
  kColumnUserLastUpdated,
  kColumnUserProgress,
  kColumnUserRating,
};

class AnimeListDialog : public win::Dialog {
public:
  AnimeListDialog();
  virtual ~AnimeListDialog() {}

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  void OnContextMenu(HWND hwnd, POINT pt);
  BOOL OnInitDialog();
  LRESULT OnListNotify(LPARAM lParam);
  LRESULT OnListCustomDraw(LPARAM lParam);
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnSize(UINT uMsg, UINT nType, SIZE size);
  LRESULT OnTabNotify(LPARAM lParam);

  int GetCurrentId();
  std::vector<int> GetCurrentIds();
  anime::Item* GetCurrentItem();
  int GetCurrentStatus();
  void RebuildIdCache();

  int GetListIndex(int anime_id);
  void RefreshList(int index = -1);
  void RefreshListItem(int anime_id);
  void RefreshListItemColumns(int index, const anime::Item& anime_item);
  void RefreshTabs(int index = -1);

  void GoToPreviousTab();
  void GoToNextTab();

public:
  // List-view control
  class ListView : public win::ListView {
  public:
    ListView();

    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void DrawAiringStatus(HDC hdc, RECT* rc, anime::Item& anime_item);
    void DrawProgressBar(HDC hdc, RECT* rc, int index, anime::Item& anime_item);
    void DrawProgressText(HDC hdc, RECT* rc, anime::Item& anime_item);
    void DrawScoreBox(HDC hdc, RECT* rc, int index, UINT uItemState, anime::Item& anime_item);

    void ExecuteAction(AnimeListAction action, int anime_id);

    int GetDefaultSortOrder(AnimeListColumn column);
    int GetSortType(AnimeListColumn column);
    void RefreshItem(int index);
    void SortFromSettings();

    class ColumnData {
    public:
      AnimeListColumn column;
      bool visible;
      int index;
      int order;
      unsigned short width;
      unsigned short width_default;
      unsigned short width_min;
      int alignment;
      std::wstring name;
      std::wstring key;
    };
    std::map<AnimeListColumn, ColumnData> columns;
    void InitializeColumns();
    void InsertColumns();
    void MoveColumn(int index, int new_visible_order);
    void SetColumnSize(int index, unsigned short width);
    AnimeListColumn FindColumnAtSubItemIndex(int index);
    void RefreshColumns(bool reset = false);
    void RefreshLastUpdateColumn();
    static AnimeListColumn TranslateColumnName(const std::wstring& name);

    win::Rect button_rect[3];
    bool button_visible[3];
    bool progress_bars_visible;
    bool dragging;
    win::ImageList drag_image;
    int hot_item;
    std::map<int, int> id_cache;
    win::Tooltip tooltips;
    AnimeListDialog* parent;
  } listview;

  // Other controls
  win::Tab tab;

private:
  int current_status_;
};

extern AnimeListDialog DlgAnimeList;

}  // namespace ui

#endif  // TAIGA_UI_DLG_ANIME_LIST_H