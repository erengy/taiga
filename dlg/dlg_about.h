/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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

#ifndef DLG_ABOUT_H
#define DLG_ABOUT_H

#include "../std.h"
#include "../ui/ui_control.h"
#include "../ui/ui_dialog.h"

// =============================================================================

/* About dialog */

class CAboutPage: public CDialog {
public:
  CAboutPage() {}
  virtual ~CAboutPage() {}

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  void OnTimer(UINT_PTR nIDEvent);
};

class CAboutWindow : public CDialog {
public:
  CAboutWindow();
  ~CAboutWindow() {}
  
  CAboutPage m_PageTaiga;
  CTab m_Tab;
  
  void OnDestroy();
  BOOL OnInitDialog();
};

extern CAboutWindow AboutWindow;

#endif // DLG_ABOUT_H