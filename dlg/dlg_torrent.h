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

#ifndef DLG_TORRENT_H
#define DLG_TORRENT_H

#include "../std.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"

// =============================================================================

class TorrentDialog : public win32::Dialog {
public:
  TorrentDialog();
  ~TorrentDialog();

  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnSize(UINT uMsg, UINT nType, SIZE size);

public:
  void ChangeStatus(wstring text, int panel_index = 0);
  void EnableInput(bool enable = true);
  void RefreshList();
  void SetTimerText(const wstring& text);

private:
  win32::ListView list_;
  win32::Rebar rebar_;
  win32::StatusBar statusbar_;
  win32::Toolbar toolbar_;
};

extern class TorrentDialog TorrentDialog;

#endif // DLG_TORRENT_H