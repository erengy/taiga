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

#ifndef WIN_RESIZABLE_H
#define WIN_RESIZABLE_H

#include "win_main.h"

namespace win32 {

// =============================================================================

class Resizable {
public:
  Resizable();
  virtual ~Resizable() {}

  void ResizeProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
  void OnInitDialog(HWND hwnd);
  void OnScroll(HWND hwnd, int nBar, UINT code);
  void OnSize(HWND hwnd, UINT nType, SIZE size);
  void ScrollClient(HWND hwnd, int nBar, int pos);

  int x_, y_;
};

} // namespace win32

#endif // WIN_RESIZABLE_H