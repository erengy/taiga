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

#include "dlg_anime_info_page.h"

enum AnimeDialogMode {
  DIALOG_MODE_ANIME_INFORMATION,
  DIALOG_MODE_NOW_PLAYING
};

// =============================================================================

class AnimeDialog : public win32::Dialog {
public:
  AnimeDialog();
  virtual ~AnimeDialog() {}

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnOK();
  void OnPaint(HDC hdc, LPPAINTSTRUCT lpps);
  void OnSize(UINT uMsg, UINT nType, SIZE size);
  BOOL PreTranslateMessage(MSG* pMsg);

  bool IsTabVisible() const;
  int GetCurrentId() const;
  void SetCurrentId(int anime_id);
  void SetCurrentPage(int index);
  void Refresh(bool image = true,
               bool series_info = true,
               bool my_info = true,
               bool connect = true);
  void UpdateControlPositions(const SIZE* size = nullptr);

public:
  PageSeriesInfo page_series_info;
  PageMyInfo page_my_info;

protected:
  int anime_id_;
  int current_page_;
  int mode_;

  class ImageLabel : public win32::Window {
  public:
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  } image_label_;

  win32::Edit edit_title_;
  win32::SysLink sys_link_;
  
  class Tab : public win32::Tab {
  public:
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void OnPaint(HDC hdc, LPPAINTSTRUCT lpps);
  } tab_;
};

class NowPlayingDialog : public AnimeDialog {
public:
  NowPlayingDialog();
  virtual ~NowPlayingDialog() {}
};

extern class AnimeDialog AnimeDialog;
extern class NowPlayingDialog NowPlayingDialog;

#endif // DLG_ANIME_INFO_H