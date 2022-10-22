/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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

#pragma once

#include <windows/win/common_controls.h>
#include <windows/win/dialog.h>

#include "track/feed_filter.h"

namespace ui {

class FeedConditionDialog : public win::Dialog {
public:
  FeedConditionDialog();
  ~FeedConditionDialog() {}

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  void OnCancel();
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  void OnOK();
  void OnPaint(HDC hdc, LPPAINTSTRUCT lpps);

  void ChooseElement(int index);

  track::FeedFilterCondition condition;

private:
  win::ComboBox element_combo_, operator_combo_, value_combo_;
};

extern FeedConditionDialog DlgFeedCondition;

}  // namespace ui
