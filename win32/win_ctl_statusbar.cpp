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

void CStatusBar::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = NULL;
  cs.lpszClass = STATUSCLASSNAME;
  cs.style     = WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | SBARS_TOOLTIPS;
}

BOOL CStatusBar::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
  m_iWidth.clear();
  return TRUE;
}

// =============================================================================

int CStatusBar::InsertPart(int iImage, int iStyle, int iAutosize, int iWidth, LPCWSTR lpText, LPCWSTR lpTooltip) {
  m_iWidth.push_back(iWidth);
  int nParts = ::SendMessage(m_hWindow, SB_GETPARTS, 0, 0);

  ::SendMessage(m_hWindow, SB_SETPARTS,   nParts + 1, reinterpret_cast<LPARAM>(&m_iWidth[0]));
  ::SendMessage(m_hWindow, SB_SETTEXT,    nParts - 1, reinterpret_cast<LPARAM>(lpText));
  ::SendMessage(m_hWindow, SB_SETTIPTEXT, nParts - 1, reinterpret_cast<LPARAM>(lpTooltip));

  if (iImage > -1 && m_hImageList) {
    HICON hIcon = ::ImageList_GetIcon(m_hImageList, iImage, 0);
    ::SendMessage(m_hWindow, SB_SETICON, nParts, reinterpret_cast<LPARAM>(hIcon));
  }

  return nParts;
}
void CStatusBar::SetImageList(HIMAGELIST hImageList) {
  m_hImageList = hImageList;
}

void CStatusBar::SetPanelText(int iPanel, LPCWSTR lpText) {
  ::SendMessage(m_hWindow, SB_SETTEXT, iPanel, reinterpret_cast<LPARAM>(lpText));
}

void CStatusBar::SetPanelTipText(int iPanel, LPCWSTR lpTipText) {
  ::SendMessage(m_hWindow, SB_SETTIPTEXT, iPanel, reinterpret_cast<LPARAM>(lpTipText));
}