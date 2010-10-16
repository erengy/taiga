/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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
#include "../ui/ui_control.h"
#include "../ui/ui_dialog.h"
#include "../ui/ui_gdi.h"

class CAnime;

// =============================================================================

class CAnimeWindow : public CDialog {
public:
  CAnimeWindow();
  virtual ~CAnimeWindow();

  // Functions
  void Refresh(CAnime* pAnimeItem = NULL);
  
  // Controls
  CEdit m_Edit;
  CToolbar m_Toolbar;

  // Image class
  class CAnimeImage {
  public:
    bool Load(wstring file);
    int Height, Width;
    CDC DC;
    CRect Rect;
  } AnimeImage;

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  void OnPaint(HDC hdc, LPPAINTSTRUCT lpps);
  BOOL PreTranslateMessage(MSG* pMsg);

private:
  CAnime* m_pAnimeItem;
  HBRUSH m_hbrDarkBlue, m_hbrLightBlue;
  HFONT m_hfTitle;
};

extern CAnimeWindow AnimeWindow;

#endif // DLG_ANIME_INFO_H