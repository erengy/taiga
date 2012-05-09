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

#ifndef DLG_ANIME_INFO_H
#define DLG_ANIME_INFO_H

#include "../std.h"
#include "../gfx.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"

class Anime;
class AnimeInfoPage;

// =============================================================================

class AnimeDialog : public win32::Dialog {
public:
  AnimeDialog();
  virtual ~AnimeDialog();

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnOK();
  void OnPaint(HDC hdc, LPPAINTSTRUCT lpps);
  BOOL PreTranslateMessage(MSG* pMsg);

public:
  int GetCurrentId() const;
  void SetCurrentPage(int index);
  void Refresh(int anime_id, bool series_info = true, bool my_info = true);

public:
  vector<AnimeInfoPage> pages;

private:
  int anime_id_;
  HBRUSH brush_darkblue_, brush_lightblue_;
  int current_page_;
  Image image_;
  win32::Tab tab_;
  HFONT title_font_;
};

extern class AnimeDialog AnimeDialog;

#endif // DLG_ANIME_INFO_H