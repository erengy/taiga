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

#ifndef TAIGA_UI_DLG_ANIME_INFO_H
#define TAIGA_UI_DLG_ANIME_INFO_H

#include "base/gfx.h"
#include "ui/dlg/dlg_anime_info_page.h"
#include "win/ctrl/win_ctrl.h"
#include "win/win_dialog.h"

namespace ui {

enum AnimeDialogMode {
  kDialogModeAnimeInformation,
  kDialogModeNowPlaying
};

class AnimeDialog : public win::Dialog {
public:
  AnimeDialog();
  virtual ~AnimeDialog() {}

  typedef std::vector<std::pair<int, double>> sorted_scores_t;

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  void OnCancel();
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnOK();
  void OnPaint(HDC hdc, LPPAINTSTRUCT lpps);
  void OnSize(UINT uMsg, UINT nType, SIZE size);
  BOOL PreTranslateMessage(MSG* pMsg);

  bool IsTabVisible() const;
  void GoToPreviousTab();
  void GoToNextTab();
  int GetMode() const;
  int GetCurrentId() const;
  void SetCurrentId(int anime_id);
  void SetCurrentPage(int index);
  void SetScores(const sorted_scores_t& scores);
  void Refresh(bool image = true,
               bool series_info = true,
               bool my_info = true,
               bool connect = true);
  void UpdateControlPositions(const SIZE* size = nullptr);
  void UpdateTitle(bool refreshing = false);

public:
  PageSeriesInfo page_series_info;
  PageMyInfo page_my_info;

protected:
  int anime_id_;
  int current_page_;
  int mode_;
  sorted_scores_t scores_;

  class ImageLabel : public win::Window {
  public:
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    AnimeDialog* parent;
  } image_label_;

  class EditTitle : public win::Edit {
  public:
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  } edit_title_;

  win::SysLink sys_link_;

  class Tab : public win::Tab {
  public:
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void OnPaint(HDC hdc, LPPAINTSTRUCT lpps);
  } tab_;
};

class NowPlayingDialog : public AnimeDialog {
public:
  NowPlayingDialog();
  ~NowPlayingDialog() {}
};

extern AnimeDialog DlgAnime;
extern NowPlayingDialog DlgNowPlaying;

}  // namespace ui

#endif  // TAIGA_UI_DLG_ANIME_INFO_H