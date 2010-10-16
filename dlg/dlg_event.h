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

#ifndef DLG_EVENT_H
#define DLG_EVENT_H

#include "../std.h"
#include "../ui/ui_control.h"
#include "../ui/ui_dialog.h"

// =============================================================================

/* Event dialog */

class CEventWindow : public CDialog {
public:
  CEventWindow();
  ~CEventWindow() {}

  CListView m_List;

  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnOK();
  void RefreshList();
};

extern CEventWindow EventWindow;

#endif // DLG_EVENT_H