/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#ifndef TAIGA_WIN_RESIZABLE_H
#define TAIGA_WIN_RESIZABLE_H

#include "win_main.h"

namespace win {

class Resizable {
public:
  Resizable();
  Resizable(bool horizontal, bool vertical);
  virtual ~Resizable() {}

  void ResizeProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
  void OnInitDialog(HWND hwnd);
  void OnScroll(HWND hwnd, int bar, UINT code);
  void OnSize(HWND hwnd, UINT type, SIZE size);
  void ScrollClient(HWND hwnd, int bar, int pos);

  bool horizontal_, vertical_;
  int x_, y_;
};

}  // namespace win

#endif  // TAIGA_WIN_RESIZABLE_H