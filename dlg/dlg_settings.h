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

#ifndef DLG_SETTINGS_H
#define DLG_SETTINGS_H

#include "../std.h"
#include "dlg_settings_page.h"
#include "../rss.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"

// =============================================================================

class CSettingsWindow : public CDialog {
public:
  CSettingsWindow();
  virtual ~CSettingsWindow();

  class CSettingsTree : public CTreeView {
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  } m_Tree;

  void RefreshTwitterLink();

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  void OnOK();
  void SetCurrentPage(int index);

private:
  int m_iCurrentPage;
  HFONT m_hStaticFont;
  vector<CSettingsPage> m_Page;
};

extern CSettingsWindow SettingsWindow;

#endif // DLG_SETTINGS_H