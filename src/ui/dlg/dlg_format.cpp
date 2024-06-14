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

#include "ui/dlg/dlg_format.h"

#include "base/html.h"
#include "base/string.h"
#include "taiga/dummy.h"
#include "taiga/resource.h"
#include "taiga/script.h"
#include "taiga/settings.h"
#include "ui/menu.h"

namespace ui {

FormatDialog DlgFormat;

FormatDialog::FormatDialog()
    : mode(FormatDialogMode::Http) {
  RegisterDlgClass(L"TaigaFormatW");
}

BOOL FormatDialog::OnInitDialog() {
  // Set up rich edit control
  rich_edit_.Attach(GetDlgItem(IDC_RICHEDIT_FORMAT));
  rich_edit_.SetEventMask(ENM_CHANGE);

  // Set text
  switch (mode) {
    case FormatDialogMode::Http:
      SetText(L"Edit Format - HTTP Request");
      rich_edit_.SetText(taiga::settings.GetShareHttpFormat().c_str());
      break;
    case FormatDialogMode::Mirc:
      SetText(L"Edit Format - mIRC");
      rich_edit_.SetText(taiga::settings.GetShareMircFormat().c_str());
      break;
    case FormatDialogMode::Balloon:
      SetText(L"Edit Format - Episode Notifications");
      rich_edit_.SetText(taiga::settings.GetSyncNotifyFormat().c_str());
      break;
  }

  return TRUE;
}

void FormatDialog::OnOK() {
  switch (mode) {
    case FormatDialogMode::Http:
      taiga::settings.SetShareHttpFormat(rich_edit_.GetText());
      break;
    case FormatDialogMode::Mirc:
      taiga::settings.SetShareMircFormat(rich_edit_.GetText());
      break;
    case FormatDialogMode::Balloon:
      taiga::settings.SetSyncNotifyFormat(rich_edit_.GetText());
      break;
  }

  EndDialog(IDOK);
}

BOOL FormatDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (LOWORD(wParam)) {
    // Add button
    case IDHELP: {
      std::wstring answer = ui::Menus.Show(GetWindowHandle(), 0, 0, L"ScriptAdd");
      if (!answer.empty()) {
        std::wstring str;
        rich_edit_.GetText(str);
        CHARRANGE cr = {0};
        rich_edit_.GetSel(&cr);
        str.replace(cr.cpMin, cr.cpMax - cr.cpMin, answer);
        rich_edit_.SetText(str.c_str());
        int pos = cr.cpMin + answer.size();
        rich_edit_.SetSel(pos, pos);
        rich_edit_.SetFocus();
      }
      return TRUE;
    }

    // Reset button
    case IDABORT: {
      base::Settings::value_t value;
      switch (mode) {
        case FormatDialogMode::Http:
          value = taiga::settings.GetDefaultValue(
              taiga::AppSettingKey::ShareHttpFormat);
          break;
        case FormatDialogMode::Mirc:
          value = taiga::settings.GetDefaultValue(
              taiga::AppSettingKey::ShareMircFormat);
          break;
        case FormatDialogMode::Balloon:
          value = taiga::settings.GetDefaultValue(
              taiga::AppSettingKey::SyncNotifyFormat);
          break;
      }
      if (base::GetSettingValueType(value) == base::SettingValueType::Wstring) {
        rich_edit_.SetText(std::get<std::wstring>(value));
      }
      return TRUE;
    }
  }

  if (LOWORD(wParam) == IDC_RICHEDIT_FORMAT &&
      HIWORD(wParam) == EN_CHANGE) {
    // Set preview text
    RefreshPreviewText();
    // Highlight
    ColorizeText();
    return TRUE;
  }

  return FALSE;
}

////////////////////////////////////////////////////////////////////////////////

void FormatDialog::ColorizeText() {
  // Save old selection
  CHARRANGE cr = {0};
  rich_edit_.GetSel(&cr);
  rich_edit_.SetRedraw(FALSE);
  rich_edit_.HideSelection(TRUE);

  // Set up character format
  CHARFORMAT cf = {0};
  cf.cbSize = sizeof(CHARFORMAT);
  cf.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE;
  cf.yHeight = 200;
  lstrcpy(cf.szFaceName, L"Courier New");

  // Get current text
  std::wstring text;
  rich_edit_.GetText(text);

  // Reset all colors
  cf.crTextColor = ::GetSysColor(COLOR_WINDOWTEXT);
  rich_edit_.SetCharFormat(SCF_ALL, &cf);

  // Highlight
  for (size_t i = 0; i < text.length(); i++) {
    switch (text[i]) {
      // Highlight functions
      case '$': {
        cf.crTextColor = RGB(0, 0, 160);
        size_t pos = text.find('(', i);
        if (pos != std::wstring::npos) {
          if (IsScriptFunction(text.substr(i + 1, pos - (i + 1)))) {
            rich_edit_.SetSel(i, pos);
            rich_edit_.SetCharFormat(SCF_SELECTION, &cf);
            i = pos;
          }
        }
        break;
      }
      // Highlight keywords
      case '%': {
        cf.crTextColor = RGB(0, 160, 0);
        size_t pos = text.find('%', i + 1);
        if (pos != std::wstring::npos) {
          if (IsScriptVariable(text.substr(i + 1, pos - (i + 1)))) {
            rich_edit_.SetSel(i, pos + 1);
            rich_edit_.SetCharFormat(SCF_SELECTION, &cf);
            i = pos;
          }
        }
        break;
      }
    }
  }

  // Reset to old selection
  rich_edit_.SetSel(&cr);
  rich_edit_.HideSelection(FALSE);
  rich_edit_.SetRedraw(TRUE);
  rich_edit_.InvalidateRect();
}

void FormatDialog::RefreshPreviewText() {
  // Replace variables
  std::wstring str;
  GetDlgItemText(IDC_RICHEDIT_FORMAT, str);
  anime::Episode* episode = &taiga::dummy_episode;
  if (CurrentEpisode.anime_id > 0)
    episode = &CurrentEpisode;
  str = ReplaceVariables(str, *episode, false, false, true);

  switch (mode) {
    case FormatDialogMode::Mirc: {
      // Strip IRC characters
      for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == 0x02 ||  // Bold
            str[i] == 0x16 ||  // Reverse
            str[i] == 0x1D ||  // Italic
            str[i] == 0x1F ||  // Underline
            str[i] == 0x0F) {  // Disable all
          str.erase(i, 1);
          i--; continue;
        }
        // Color code
        if (str[i] == 0x03) {
          str.erase(i, 1);
          if (IsNumericChar(str[i]))
            str.erase(i, 1);
          if (IsNumericChar(str[i]))
            str.erase(i, 1);
          i--;
          continue;
        }
      }
      break;
    }
  }

  // Set final text
  SetDlgItemText(IDC_EDIT_PREVIEW, str.c_str());
}

}  // namespace ui
