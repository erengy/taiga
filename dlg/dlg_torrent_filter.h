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

#ifndef DLG_TORRENT_FILTER_H
#define DLG_TORRENT_FILTER_H

#include "../std.h"
#include "../feed.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"

// =============================================================================

class CTorrentFilterWindow : public CDialog {
public:
  CTorrentFilterWindow();
  virtual ~CTorrentFilterWindow();

  CComboBox m_Combo;
  CFeedFilter m_Filter;

  void RefreshComboBox(int type = 0);
  void SetValues(int option, int type, wstring value);
  
  BOOL DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  void OnCancel();
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  void OnOK();

private:
  wstring m_LastKeyword;
  int m_iLastType;
};

extern CTorrentFilterWindow TorrentFilterWindow;

#endif // DLG_TORRENT_FILTER_H