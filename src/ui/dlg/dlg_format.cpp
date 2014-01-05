/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#include "base/string.h"
#include "taiga/dummy.h"
#include "taiga/resource.h"
#include "taiga/script.h"
#include "taiga/settings.h"
#include "ui/dlg/dlg_format.h"
#include "ui/menu.h"

namespace ui {

FormatDialog DlgFormat;

FormatDialog::FormatDialog()
    : mode(kFormatModeHttp) {
  RegisterDlgClass(L"TaigaFormatW");
}

BOOL FormatDialog::OnInitDialog() {
  // Set up rich edit control
  rich_edit_.Attach(GetDlgItem(IDC_RICHEDIT_FORMAT));
  rich_edit_.SetEventMask(ENM_CHANGE);

  // Set text
  switch (mode) {
    case kFormatModeHttp:
      SetText(L"Edit format - HTTP request");
      rich_edit_.SetText(Settings[taiga::kShare_Http_Format].c_str());
      break;    
    case kFormatModeMessenger:
      SetText(L"Edit format - Messenger");
      rich_edit_.SetText(Settings[taiga::kShare_Messenger_Format].c_str());
      break;
    case kFormatModeMirc:
      SetText(L"Edit format - mIRC");
      rich_edit_.SetText(Settings[taiga::kShare_Mirc_Format].c_str());
      break;
    case kFormatModeSkype:
      SetText(L"Edit format - Skype");
      rich_edit_.SetText(Settings[taiga::kShare_Skype_Format].c_str());
      break;
    case kFormatModeTwitter:
      SetText(L"Edit format - Twitter");
      rich_edit_.SetText(Settings[taiga::kShare_Twitter_Format].c_str());
      break;
    case kFormatModeBalloon:
      SetText(L"Edit format - Balloon tooltips");
      rich_edit_.SetText(Settings[taiga::kSync_Notify_Format].c_str());
      break;
  }

  return TRUE;
}

void FormatDialog::OnOK() {
  switch (mode) {
    case kFormatModeHttp:
      Settings.Set(taiga::kShare_Http_Format, rich_edit_.GetText());
      break;
    case kFormatModeMessenger:
      Settings.Set(taiga::kShare_Messenger_Format, rich_edit_.GetText());
      break;
    case kFormatModeMirc:
      Settings.Set(taiga::kShare_Mirc_Format, rich_edit_.GetText());
      break;
    case kFormatModeSkype:
      Settings.Set(taiga::kShare_Skype_Format, rich_edit_.GetText());
      break;
    case kFormatModeTwitter:
      Settings.Set(taiga::kShare_Twitter_Format, rich_edit_.GetText());
      break;
    case kFormatModeBalloon:
      Settings.Set(taiga::kSync_Notify_Format, rich_edit_.GetText());
      break;
  }

  EndDialog(IDOK);
}

BOOL FormatDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (LOWORD(wParam)) {
    // Add button    
    case IDHELP: {
      std::wstring answer = ui::Menus.Show(GetWindowHandle(), 0, 0, L"ScriptAdd");
      std::wstring str;
      rich_edit_.GetText(str);
      CHARRANGE cr = {0};
      rich_edit_.GetSel(&cr);
      str.insert(cr.cpMin, answer);
      rich_edit_.SetText(str.c_str());
      rich_edit_.SetFocus();
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
        if (pos != wstring::npos) {
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
        if (pos != wstring::npos) {
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
  anime::Episode* episode = &taiga::DummyEpisode;
  if (CurrentEpisode.anime_id > 0)
    episode = &CurrentEpisode;
  str = ReplaceVariables(str, *episode, false, false, true);

  switch (mode) {
    case kFormatModeMirc: {
      // Strip IRC characters
      for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == 0x02 || // Bold
            str[i] == 0x16 || // Reverse
            str[i] == 0x1D || // Italic
            str[i] == 0x1F || // Underline
            str[i] == 0x0F) { // Disable all
          str.erase(i, 1);
          i--; continue;
        }
        // Color code
        if (str[i] == 0x03) {
          str.erase(i, 1);
          if (IsNumeric(str[i]))
            str.erase(i, 1);
          if (IsNumeric(str[i]))
            str.erase(i, 1);
          i--;
          continue;
        }
      }
      break;
    }
    case kFormatModeSkype: {
      // Strip HTML codes
      StripHtmlTags(str);
      break;
    }
  }

  // Set final text
  SetDlgItemText(IDC_EDIT_PREVIEW, str.c_str());
}

}  // namespace ui