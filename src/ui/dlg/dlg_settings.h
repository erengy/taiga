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

#include <map>
#include <vector>

#include "track/feed.h"
#include <windows/win/common_controls.h>
#include <windows/win/dialog.h>
#include "ui/dlg/dlg_settings_page.h"

namespace ui {

enum SettingsSections {
  kSettingsSectionServices = 1,
  kSettingsSectionLibrary,
  kSettingsSectionApplication,
  kSettingsSectionRecognition,
  kSettingsSectionSharing,
  kSettingsSectionTorrents,
  kSettingsSectionAdvanced,
};

class SettingsDialog : public win::Dialog {
public:
  SettingsDialog();
  ~SettingsDialog() {}

  friend class SettingsPage;

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnOK();

  void SetCurrentSection(SettingsSections section);
  void SetCurrentPage(SettingsPages page);

  int AddTorrentFilterToList(HWND hwnd_list, const FeedFilter& filter);
  void RefreshCache();
  void RefreshTorrentFilterList(HWND hwnd_list);
  void RefreshTwitterLink();

private:
  class TreeView : public win::TreeView {
  public:
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    std::map<int, HTREEITEM> items;
  } tree_;
  win::Tab tab_;

private:
  std::map<int, std::pair<std::wstring, std::wstring>> advanced_settings_;
  SettingsSections current_section_;
  SettingsPages current_page_;
  std::vector<FeedFilter> feed_filters_;
  std::vector<SettingsPage> pages;
};

extern SettingsDialog DlgSettings;

}  // namespace ui
