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

#include "win_control.h"

// =============================================================================

void CProgressBar::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = 0;
  cs.lpszClass = PROGRESS_CLASS;
  cs.style     = WS_CHILD | WS_VISIBLE;
}

void CProgressBar::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
  CWindow::OnCreate(hwnd, lpCreateStruct);
}

// =============================================================================

UINT CProgressBar::GetPosition() {
  return ::SendMessage(m_hWindow, PBM_GETPOS, 0, 0);
}

void CProgressBar::SetMarquee(bool enabled) {
  if (enabled) {
    SetStyle(PBS_MARQUEE, 0);
  } else {
    SetStyle(0, PBS_MARQUEE);
  }
  ::SendMessage(m_hWindow, PBM_SETMARQUEE, enabled, 0);
}

UINT CProgressBar::SetPosition(UINT position) {
  return ::SendMessage(m_hWindow, PBM_SETPOS, position, 0);
}

DWORD CProgressBar::SetRange(UINT min, UINT max) {
  return ::SendMessage(m_hWindow, PBM_SETRANGE32, min, max);
}

UINT CProgressBar::SetState(UINT state) {
  return ::SendMessage(m_hWindow, PBM_SETSTATE, state, 0);
}