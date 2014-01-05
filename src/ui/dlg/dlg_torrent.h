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

#ifndef DLG_TORRENT_H
#define DLG_TORRENT_H

#include "base/std.h"
#include "win/ctrl/win_ctrl.h"
#include "win/win_dialog.h"

// =============================================================================

class TorrentDialog : public win::Dialog {
public:
  TorrentDialog() {};
  virtual ~TorrentDialog() {};

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnSize(UINT uMsg, UINT nType, SIZE size);

public:
  void EnableInput(bool enable = true);
  void RefreshList();
  void Search(wstring url, int anime_id);
  void Search(wstring url, wstring title);
  void SetTimerText(const wstring& text);

private:
  win::ListView list_;
  win::Rebar rebar_;
  win::Toolbar toolbar_;
};

extern class TorrentDialog TorrentDialog;

#endif // DLG_TORRENT_H