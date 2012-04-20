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

#ifndef DLG_SEARCH_H
#define DLG_SEARCH_H

#include "../std.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"

// =============================================================================

class SearchDialog : public win32::Dialog {
public:
  SearchDialog();
  ~SearchDialog() {};
  
  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnOK();
  BOOL PreTranslateMessage(MSG* pMsg);

public:
  void EnableInput(bool enable);
  void ParseResults(const wstring& data);
  void RefreshList();
  bool Search(const wstring& title);

private:
  vector<int> anime_ids_;
  win32::Edit edit_;
  win32::ListView list_;
};

extern class SearchDialog SearchDialog;

#endif // DLG_SEARCH_H