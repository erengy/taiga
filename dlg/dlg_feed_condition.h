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

#ifndef DLG_FEED_CONDITION_H
#define DLG_FEED_CONDITION_H

#include "../std.h"
#include "../feed.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"

// =============================================================================

class CFeedConditionWindow : public CDialog {
public:
  CFeedConditionWindow();
  virtual ~CFeedConditionWindow();

  BOOL DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  void OnCancel();
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  void OnOK();
  void OnPaint(HDC hdc, LPPAINTSTRUCT lpps);

  void ChooseElement(int index);

public:
  CComboBox element_combo_;
  CComboBox operator_combo_;
  CComboBox value_combo_;

  CFeedFilterCondition condition_;
};

extern CFeedConditionWindow FeedConditionWindow;

#endif // DLG_FEED_CONDITION_H