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

#include "win_ctrl.h"

namespace win {

ProgressBar::ProgressBar(HWND hwnd) {
  SetWindowHandle(hwnd);
}

void ProgressBar::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = 0;
  cs.lpszClass = PROGRESS_CLASS;
  cs.style = WS_CHILD | WS_VISIBLE;
}

void ProgressBar::OnCreate(HWND hwnd, LPCREATESTRUCT create_struct) {
  Window::OnCreate(hwnd, create_struct);
}

////////////////////////////////////////////////////////////////////////////////

UINT ProgressBar::GetPosition() {
  return SendMessage(PBM_GETPOS, 0, 0);
}

void ProgressBar::SetMarquee(bool enabled) {
  if (enabled) {
    SetStyle(PBS_MARQUEE, 0);
  } else {
    SetStyle(0, PBS_MARQUEE);
  }

  SendMessage(PBM_SETMARQUEE, enabled, 0);
}

UINT ProgressBar::SetPosition(UINT position) {
  return SendMessage(PBM_SETPOS, position, 0);
}

DWORD ProgressBar::SetRange(UINT min, UINT max) {
  return SendMessage(PBM_SETRANGE32, min, max);
}

UINT ProgressBar::SetState(UINT state) {
  return SendMessage(PBM_SETSTATE, state, 0);
}

}  // namespace win