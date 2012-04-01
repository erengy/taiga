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

#ifndef DLG_SETTINGS_H
#define DLG_SETTINGS_H

#include "../std.h"
#include "dlg_settings_page.h"
#include "../feed.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"

// =============================================================================

class SettingsDialog : public CDialog {
public:
  SettingsDialog();
  virtual ~SettingsDialog();

  friend class SettingsPage;

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  void OnOK();
  void SetCurrentPage(int index);

public:
  int AddTorrentFilterToList(HWND hwnd_list, const FeedFilter& filter);
  void RefreshTorrentFilterList(HWND hwnd_list);
  void RefreshTwitterLink();

private:
  class TreeView : public CTreeView {
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  } tree_;

private:
  int current_page_;
  vector<FeedFilter> feed_filters_;
  HFONT static_font_;
  vector<SettingsPage> pages;
};

extern SettingsDialog SettingsDialog;

#endif // DLG_SETTINGS_H