/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#pragma once

#include <windows/win/dialog.h>

namespace ui {

enum SettingsPages {
  kSettingsPageAdvanced = 1,
  kSettingsPageAppGeneral,
  kSettingsPageAppList,
  kSettingsPageLibraryCache,
  kSettingsPageLibraryFolders,
  kSettingsPageRecognitionGeneral,
  kSettingsPageRecognitionMedia,
  kSettingsPageRecognitionStream,
  kSettingsPageServicesKitsu,
  kSettingsPageServicesMain,
  kSettingsPageServicesMal,
  kSettingsPageSharingHttp,
  kSettingsPageSharingMirc,
  kSettingsPageSharingSkype,
  kSettingsPageSharingTwitter,
  kSettingsPageTorrentsDiscovery,
  kSettingsPageTorrentsDownloads,
  kSettingsPageTorrentsFilters,
  kSettingsPageCount
};

class SettingsDialog;

class SettingsPage : public win::Dialog {
public:
  SettingsPage();
  virtual ~SettingsPage() {}

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  void OnDropFiles(HDROP hDropInfo);
  BOOL OnInitDialog();
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);

  void Create();

  int index;
  SettingsDialog* parent;
};

}  // namespace ui
