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

#ifndef DLG_SETTINGS_PAGE_H
#define DLG_SETTINGS_PAGE_H

#include "base/std.h"
#include "win32/win_dialog.h"

enum SettingsPages {
  PAGE_APP_BEHAVIOR = 1,
  PAGE_APP_CONNECTION,
  PAGE_APP_INTERFACE,
  PAGE_APP_LIST,
  PAGE_LIBRARY_CACHE,
  PAGE_LIBRARY_FOLDERS,
  PAGE_RECOGNITION_GENERAL,
  PAGE_RECOGNITION_MEDIA,
  PAGE_RECOGNITION_STREAM,
  PAGE_SERVICES_MAL,
  PAGE_SHARING_HTTP,
  PAGE_SHARING_MESSENGER,
  PAGE_SHARING_MIRC,
  PAGE_SHARING_SKYPE,
  PAGE_SHARING_TWITTER,
  PAGE_TORRENTS_DISCOVERY,
  PAGE_TORRENTS_DOWNLOADS,
  PAGE_TORRENTS_FILTERS,
  PAGE_COUNT
};

class SettingsDialog;

// =============================================================================

class SettingsPage : public win32::Dialog {
public:
  SettingsPage();
  virtual ~SettingsPage() {}

  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  void OnDropFiles(HDROP hDropInfo);
  BOOL OnInitDialog();
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);

  void Create();

  int index;
  SettingsDialog* parent;
};

#endif // DLG_SETTINGS_PAGE_H