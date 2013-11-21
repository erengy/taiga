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

#include "../std.h"

#include "dlg_main.h"
#include "dlg_update.h"
#include "dlg_update_new.h"

#include "../common.h"
#include "../foreach.h"
#include "../http.h"
#include "../resource.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"

#include "../win32/win_gdi.h"

class NewUpdateDialog NewUpdateDialog;

// =============================================================================

NewUpdateDialog::NewUpdateDialog() {
}

BOOL NewUpdateDialog::OnInitDialog() {
  // Set icon
  SetIconLarge(IDI_MAIN);
  SetIconSmall(IDI_MAIN);

  // Set main text
  SendDlgItemMessage(IDC_STATIC_UPDATE_TITLE, WM_SETFONT, 
                     reinterpret_cast<WPARAM>(UI.font_header.Get()), TRUE);
  SetDlgItemText(IDC_STATIC_UPDATE_TITLE,
                 L"A new version of " APP_NAME L" is available!");
  
  // Set details text
  SetDlgItemText(IDC_STATIC_UPDATE_DETAILS, L"Current version: " APP_VERSION);
  
  // Set changelog text
  auto current_version = Taiga.Updater.GetVersionValue(
      Taiga.GetVersionMajor(), Taiga.GetVersionMinor(), Taiga.GetVersionRevision());
  wstring changelog =
      L"{\\rtf1\\ansi\\deff0"
      L"{\\fonttbl"
      L"{\\f0 Segoe UI;}"
      L"}"
      L"\\deflang1024\\fs18";
  foreach_(item, Taiga.Updater.items) {
    vector<wstring> numbers;
    Split(item->guid, L".", numbers);
    auto item_version = Taiga.Updater.GetVersionValue(
        ToInt(numbers.at(0)), ToInt(numbers.at(1)), ToInt(numbers.at(2)));
    if (item_version > current_version) {
      changelog +=  L"\\b Version " + item->guid + L"\\b0\\line ";
      wstring description = item->description;
      Replace(description, L"\n", L"\\line ", true);
      changelog += description + L"\\line\\line ";
    }
  }
  changelog += L"}";
  win32::RichEdit rich_edit(GetDlgItem(IDC_RICHEDIT_UPDATE));
  rich_edit.SetTextEx(ToANSI(changelog));
  rich_edit.SetWindowHandle(nullptr);

  return TRUE;
}

void NewUpdateDialog::OnOK() {
  EndDialog(IDOK);

  if (!Taiga.Updater.Download())
    UpdateDialog.PostMessage(WM_CLOSE);
}

void NewUpdateDialog::OnCancel() {
  EndDialog(IDCANCEL);

  UpdateDialog.PostMessage(WM_CLOSE);
}