/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#include <windows/win/dialog.h>

namespace ui {

class NewUpdateDialog : public win::Dialog {
public:
  NewUpdateDialog() {}
  ~NewUpdateDialog() {}

  BOOL OnInitDialog();
  void OnOK();
  void OnCancel();
  void OnPaint(HDC hdc, LPPAINTSTRUCT lpps);
};

extern NewUpdateDialog DlgUpdateNew;

}  // namespace ui
