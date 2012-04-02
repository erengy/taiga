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

#ifndef DLG_INPUT_H
#define DLG_INPUT_H

#include "../std.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"

// =============================================================================

class InputDialog : public win32::Dialog {
public:
  InputDialog();
  virtual ~InputDialog() {}

  void OnCancel();
  BOOL OnInitDialog();
  void OnOK();

public:
  void SetNumbers(bool enabled, int min_value, int max_value, int current_value);
  void Show(HWND parent = nullptr);

public:
  INT_PTR result;
  wstring info, title, text;

private:
  int current_value_, min_value_, max_value_;
  bool numbers_only_;
  win32::Edit edit_;
  win32::Spin spin_;
};

#endif // DLG_INPUT_H