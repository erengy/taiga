/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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

#include "ui/dialog.h"

#include "base/process.h"
#include "taiga/resource.h"
#include "ui/dlg/dlg_about.h"
#include "ui/dlg/dlg_anime_info.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_season.h"
#include "ui/dlg/dlg_settings.h"
#include "ui/dlg/dlg_torrent.h"
#include "ui/dlg/dlg_update.h"

namespace ui {

struct DialogProperties {
  unsigned int resource_id = 0;
  win::Dialog* dialog = nullptr;
  Dialog parent = Dialog::None;
  bool modal = false;
};

static std::map<Dialog, DialogProperties> dialog_properties;

void InitializeDialogProperties() {
  if (!dialog_properties.empty())
    return;

  dialog_properties.insert(std::make_pair(
      Dialog::About,
      DialogProperties{IDD_ABOUT, &DlgAbout, Dialog::Main, true}));
  dialog_properties.insert(std::make_pair(
      Dialog::AnimeInformation,
      DialogProperties{IDD_ANIME_INFO, &DlgAnime, Dialog::Main, false}));
  dialog_properties.insert(std::make_pair(
      Dialog::Main,
      DialogProperties{IDD_MAIN, &DlgMain, Dialog::None, false}));
  dialog_properties.insert(std::make_pair(
      Dialog::Settings,
      DialogProperties{IDD_SETTINGS, &DlgSettings, Dialog::Main, true}));
  dialog_properties.insert(std::make_pair(
      Dialog::Update,
      DialogProperties{IDD_UPDATE, &DlgUpdate, Dialog::Main, true}));
}

////////////////////////////////////////////////////////////////////////////////

void EndDialog(Dialog dialog) {
  InitializeDialogProperties();

  const auto it = dialog_properties.find(dialog);

  if (it != dialog_properties.end() && it->second.dialog)
    it->second.dialog->EndDialog(IDABORT);
}

void EnableDialogInput(Dialog dialog, bool enable) {
  switch (dialog) {
    case Dialog::Main:
      DlgMain.EnableInput(enable);
      break;
    case Dialog::Seasons:
      DlgSeason.EnableInput(enable);
      break;
    case Dialog::Torrents:
      DlgTorrent.EnableInput(enable);
      break;
  }
}

HWND GetWindowHandle(Dialog dialog) {
  InitializeDialogProperties();

  const auto it = dialog_properties.find(dialog);

  if (it != dialog_properties.end() && it->second.dialog)
    return it->second.dialog->GetWindowHandle();

  return nullptr;
}

void ShowDialog(Dialog dialog) {
  InitializeDialogProperties();

  const auto it = dialog_properties.find(dialog);

  if (it != dialog_properties.end() && it->second.dialog) {
    if (!it->second.dialog->IsWindow()) {
      it->second.dialog->Create(it->second.resource_id,
                                GetWindowHandle(it->second.parent),
                                it->second.modal);
    } else {
      ActivateWindow(it->second.dialog->GetWindowHandle());
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void ShowDlgAnimeEdit(int anime_id) {
  DlgAnime.SetCurrentId(anime_id);
  DlgAnime.SetCurrentPage(AnimePageType::MyInfo);

  ShowDialog(Dialog::AnimeInformation);
}

void ShowDlgAnimeInfo(int anime_id) {
  DlgAnime.SetCurrentId(anime_id);
  DlgAnime.SetCurrentPage(AnimePageType::SeriesInfo);

  ShowDialog(Dialog::AnimeInformation);
}

void ShowDlgSettings(int section, int page) {
  if (section > 0)
    DlgSettings.SetCurrentSection(static_cast<SettingsSections>(section));
  if (page > 0)
    DlgSettings.SetCurrentPage(static_cast<SettingsPages>(page));

  ShowDialog(Dialog::Settings);
}

}  // namespace ui
