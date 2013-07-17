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

#ifndef DLG_ANIME_LIST_H
#define DLG_ANIME_LIST_H

#include "../std.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"
#include "../win32/win_gdi.h"

namespace anime {
class Item;
}

// =============================================================================

class AnimeListDialog : public win32::Dialog {
public:
  AnimeListDialog();
  virtual ~AnimeListDialog() {}

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  LRESULT OnListNotify(LPARAM lParam);
  LRESULT OnListCustomDraw(LPARAM lParam);
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnSize(UINT uMsg, UINT nType, SIZE size);
  LRESULT OnTabNotify(LPARAM lParam);

  int GetCurrentId();
  anime::Item* GetCurrentItem();
  void SetCurrentId(int anime_id);
  int GetListIndex(int anime_id);
  void RefreshList(int index = -1);
  void RefreshListItem(int anime_id);
  void RefreshTabs(int index = -1, bool redraw = true);

public:
  // List-view control
  class ListView : public win32::ListView {
  public:
    ListView();
    
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void DrawProgressBar(HDC hdc, RECT* rc, UINT uItemState, const anime::Item* anime_item);
    int GetSortType(int column);
    void RefreshProgressButtons(const anime::Item* anime_item);
    
    win32::Rect button_rect[2];
    bool button_visible[2];
    bool dragging;
    win32::ImageList drag_image;
    win32::Tooltip tooltips;
    AnimeListDialog* parent;
  } listview;

  // Other controls
  win32::Tab tab;

private:
  int current_id_;
};

extern class AnimeListDialog AnimeListDialog;

#endif // DLG_ANIME_LIST_H