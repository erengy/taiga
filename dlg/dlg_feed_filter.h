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

#ifndef DLG_FEED_FILTER_H
#define DLG_FEED_FILTER_H

#include "../std.h"
#include "../feed.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"

// =============================================================================

class CFeedFilterWindow : public CDialog {
public:
  CFeedFilterWindow();
  virtual ~CFeedFilterWindow();
  
  BOOL DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  void OnCancel();
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  void OnOK();
  
public:
  CFeedFilter m_Filter;

private:
  void AddConditionToList(const CFeedFilterCondition& condition);

private:
  CListView m_List;
};

extern CFeedFilterWindow FeedFilterWindow;

#endif // DLG_FEED_FILTER_H