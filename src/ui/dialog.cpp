/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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
#include "ui/dlg/dlg_test_recognition.h"
#include "ui/dlg/dlg_torrent.h"
#include "ui/dlg/dlg_update.h"
#include "ui/dialog.h"

namespace ui {

void EnableDialogInput(Dialog dialog, bool enable) {
  switch (dialog) {
    case kDialogMain:
      MainDialog.EnableInput(enable);
      break;
    case kDialogTorrents:
      TorrentDialog.EnableInput(enable);
      break;
  }
}

void ShowDialog(Dialog dialog) {
  win::Dialog* dlg = nullptr;
  unsigned int resource_id = 0;
  HWND parent = g_hMain;
  bool modal = true;

  switch (dialog) {
    case kDialogAbout:
      dlg = &AboutDialog;
      resource_id = IDD_ABOUT;
      break;

    case kDialogAnimeInformation:
      dlg = &AnimeDialog;
      resource_id = IDD_ANIME_INFO;
      modal = false;
      break;

    case kDialogMain:
      dlg = &MainDialog;
      resource_id = IDD_MAIN;
      parent = nullptr;
      modal = false;
      break;

    case kDialogSettings:
      dlg = &SettingsDialog;
      resource_id = IDD_SETTINGS;
      break;

    case kDialogTestRecognition:
      dlg = &RecognitionTest;
      resource_id = IDD_TEST_RECOGNITION;
      parent = nullptr;
      modal = false;
      break;

    case kDialogUpdate:
      dlg = &UpdateDialog;
      resource_id = IDD_UPDATE;
      break;
  }

  if (dlg && resource_id) {
    if (!dlg->IsWindow()) {
      dlg->Create(resource_id, parent, modal);
    } else {
      ActivateWindow(dlg->GetWindowHandle());
    }
  }
}

}  // namespace ui