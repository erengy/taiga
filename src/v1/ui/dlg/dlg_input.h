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

#include <string>

#include <windows/win/common_controls.h>
#include <windows/win/dialog.h>

namespace ui {

class InputDialog : public win::Dialog {
public:
  InputDialog();
  ~InputDialog() {}

  void OnCancel();
  BOOL OnInitDialog();
  void OnOK();

  void SetNumbers(bool enabled, int min_value, int max_value, int current_value);
  void SetPassword(bool enabled);
  void Show(HWND parent = nullptr);

  INT_PTR result;
  std::wstring info, title, text;

private:
  int current_value_, min_value_, max_value_;
  bool numbers_only_, password_;
  win::Edit edit_;
  win::Spin spin_;
};

}  // namespace ui
