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

#ifndef DLG_SETTINGS_PAGE_H
#define DLG_SETTINGS_PAGE_H

#include "../std.h"
#include "../win32/win_dialog.h"

enum SettingsPages {
  PAGE_ACCOUNT,
  PAGE_UPDATE,
  PAGE_HTTP,
  PAGE_MESSENGER,
  PAGE_MIRC,
  PAGE_SKYPE,
  PAGE_TWITTER,
  PAGE_FOLDERS_ROOT,
  PAGE_FOLDERS_ANIME,
  PAGE_PROGRAM,
  PAGE_LIST,
  PAGE_NOTIFICATIONS,
  PAGE_MEDIA,
  PAGE_TORRENT1,
  PAGE_TORRENT2,
  PAGE_COUNT
};

class SettingsDialog;

// =============================================================================

class SettingsPage : public CDialog {
public:
  SettingsPage();
  virtual ~SettingsPage() {}

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();

public:
  void CreateItem(LPCWSTR pszText, HTREEITEM htiParent);
  void Select();

public:
  int index;
  SettingsDialog* parent;

private:
  HTREEITEM tree_item_;
};

#endif // DLG_SETTINGS_PAGE_H