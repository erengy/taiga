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

#include "base/gfx.h"
#include "base/string.h"
#include "taiga/config.h"
#include "taiga/resource.h"
#include "taiga/update.h"
#include "taiga/version.h"
#include "ui/dlg/dlg_update.h"
#include "ui/dlg/dlg_update_new.h"
#include "ui/theme.h"

namespace ui {

NewUpdateDialog DlgUpdateNew;

BOOL NewUpdateDialog::OnInitDialog() {
  // Set icon
  SetIconLarge(IDI_MAIN);
  SetIconSmall(IDI_MAIN);

  // Set main text
  SendDlgItemMessage(IDC_STATIC_UPDATE_TITLE, WM_SETFONT,
                     reinterpret_cast<WPARAM>(Theme.GetHeaderFont()), TRUE);
  SetDlgItemText(IDC_STATIC_UPDATE_TITLE,
                 L"A new version of " TAIGA_APP_NAME L" is available!");

  // Set details text
  std::wstring text = L"Current version: " + StrToWstr(taiga::version().to_string());
  SetDlgItemText(IDC_STATIC_UPDATE_DETAILS, text.c_str());

  // Set changelog text
  std::wstring changelog =
      L"{\\rtf1\\ansi\\deff0"
      L"{\\fonttbl"
      L"{\\f0 Segoe UI;}"
      L"}"
      L"\\deflang1024\\fs18";
  for (const auto& item : taiga::updater.items) {
    semaver::Version item_version(WstrToStr(item.guid.value));
    if (item_version > taiga::version()) {
      changelog += L"\\b Version " + item.guid.value + L"\\b0\\line ";
      std::wstring description = item.description;
      ReplaceString(description, L"\n", L"\\line ");
      changelog += description + L"\\line\\line ";
    }
  }
  changelog += L"}";
  win::RichEdit rich_edit(GetDlgItem(IDC_RICHEDIT_UPDATE));
  rich_edit.SetTextEx(WstrToStr(changelog));
  rich_edit.SetWindowHandle(nullptr);

  return TRUE;
}

void NewUpdateDialog::OnOK() {
  EndDialog(IDOK);

  if (!taiga::updater.Download())
    DlgUpdate.PostMessage(WM_CLOSE);
}

void NewUpdateDialog::OnCancel() {
  EndDialog(IDCANCEL);

  DlgUpdate.PostMessage(WM_CLOSE);
}

void NewUpdateDialog::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  win::Dc dc = hdc;

  // Paint application icon
  win::Rect rect;
  win::Window label = GetDlgItem(IDC_STATIC_APP_ICON);
  label.GetWindowRect(GetWindowHandle(), &rect);
  label.SetWindowHandle(nullptr);
  DrawIconResource(IDI_MAIN, dc.Get(), rect, true, false);
}

}  // namespace ui