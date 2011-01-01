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

#ifndef DLG_ANIME_INFO_H
#define DLG_ANIME_INFO_H

#include "../std.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"
#include "../win32/win_gdi.h"

class CAnime;

// =============================================================================

class CAnimeInfoPage;

class CAnimeWindow : public CDialog {
public:
  CAnimeWindow();
  virtual ~CAnimeWindow();

  // Controls
  CTab m_Tab;

  // Pages
  vector<CAnimeInfoPage> m_Page;

  // Image class
  class CAnimeImage {
  public:
    bool Load(const wstring& file);
    int Height, Width;
    CDC DC;
    CRect Rect;
  } AnimeImage;

  // Functions
  void SetCurrentPage(int index);
  void Refresh(CAnime* pAnimeItem = NULL, 
    bool series_info = true, bool my_info = true);

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnOK();
  void OnPaint(HDC hdc, LPPAINTSTRUCT lpps);
  BOOL PreTranslateMessage(MSG* pMsg);

private:
  CAnime* m_pAnimeItem;
  HBRUSH m_hbrDarkBlue, m_hbrLightBlue;
  HFONT m_hfTitle;
  int m_iCurrentPage;
};

extern CAnimeWindow AnimeWindow;

#endif // DLG_ANIME_INFO_H