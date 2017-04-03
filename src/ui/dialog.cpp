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

#include "base/process.h"
#include "taiga/resource.h"
#include "ui/dlg/dlg_about.h"
#include "ui/dlg/dlg_anime_info.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_settings.h"
#include "ui/dlg/dlg_torrent.h"
#include "ui/dlg/dlg_update.h"
#include "ui/dialog.h"

namespace ui {

class DialogProperties {
public:
  DialogProperties(unsigned int resource_id, win::Dialog* dialog,
                   Dialog parent = kDialogNone, bool modal = false);

  unsigned int resource_id;
  win::Dialog* dialog;
  Dialog parent;
  bool modal;
};

DialogProperties::DialogProperties(unsigned int resource_id,
                                   win::Dialog* dialog,
                                   Dialog parent,
                                   bool modal)
    : resource_id(resource_id),
      dialog(dialog),
      parent(parent),
      modal(modal) {
}

std::map<Dialog, DialogProperties> dialog_properties;

void InitializeDialogProperties() {
  if (!dialog_properties.empty())
    return;

  dialog_properties.insert(std::make_pair(
      kDialogAbout,
      DialogProperties(IDD_ABOUT, &DlgAbout, kDialogMain, true)));
  dialog_properties.insert(std::make_pair(
      kDialogAnimeInformation,
      DialogProperties(IDD_ANIME_INFO, &DlgAnime, kDialogMain, false)));
  dialog_properties.insert(std::make_pair(
      kDialogMain,
      DialogProperties(IDD_MAIN, &DlgMain)));
  dialog_properties.insert(std::make_pair(
      kDialogSettings,
      DialogProperties(IDD_SETTINGS, &DlgSettings, kDialogMain, true)));
  dialog_properties.insert(std::make_pair(
      kDialogUpdate,
      DialogProperties(IDD_UPDATE, &DlgUpdate, kDialogMain, true)));
}

////////////////////////////////////////////////////////////////////////////////

void EndDialog(Dialog dialog) {
  InitializeDialogProperties();

  auto it = dialog_properties.find(dialog);

  if (it != dialog_properties.end())
    if (it->second.dialog)
      it->second.dialog->EndDialog(IDABORT);
}

void EnableDialogInput(Dialog dialog, bool enable) {
  switch (dialog) {
    case kDialogMain:
      DlgMain.EnableInput(enable);
      break;
    case kDialogTorrents:
      DlgTorrent.EnableInput(enable);
      break;
  }
}

HWND GetWindowHandle(Dialog dialog) {
  InitializeDialogProperties();

  auto it = dialog_properties.find(dialog);

  if (it != dialog_properties.end())
    if (it->second.dialog)
      return it->second.dialog->GetWindowHandle();

  return nullptr;
}

void ShowDialog(Dialog dialog) {
  InitializeDialogProperties();

  auto it = dialog_properties.find(dialog);

  if (it != dialog_properties.end()) {
    if (it->second.dialog) {
      if (!it->second.dialog->IsWindow()) {
        it->second.dialog->Create(it->second.resource_id,
                                  GetWindowHandle(it->second.parent),
                                  it->second.modal);
      } else {
        ActivateWindow(it->second.dialog->GetWindowHandle());
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void ShowDlgAnimeEdit(int anime_id) {
  DlgAnime.SetCurrentId(anime_id);
  DlgAnime.SetCurrentPage(kAnimePageMyInfo);

  ShowDialog(kDialogAnimeInformation);
}

void ShowDlgAnimeInfo(int anime_id) {
  DlgAnime.SetCurrentId(anime_id);
  DlgAnime.SetCurrentPage(kAnimePageSeriesInfo);

  ShowDialog(kDialogAnimeInformation);
}

void ShowDlgSettings(int section, int page) {
  if (section > 0)
    DlgSettings.SetCurrentSection(static_cast<SettingsSections>(section));
  if (page > 0)
    DlgSettings.SetCurrentPage(static_cast<SettingsPages>(page));

  ShowDialog(kDialogSettings);
}

}  // namespace ui