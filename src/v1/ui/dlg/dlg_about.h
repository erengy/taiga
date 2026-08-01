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

namespace ui {

class AboutDialog : public win::Dialog {
public:
  AboutDialog();
  ~AboutDialog() {}

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnDestroy();
  BOOL OnInitDialog();
  void OnPaint(HDC hdc, LPPAINTSTRUCT lpps);

private:
  win::RichEdit rich_edit_;
};

extern AboutDialog DlgAbout;

}  // namespace ui
