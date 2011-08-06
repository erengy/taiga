/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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
#include "../animelist.h"
#include "../common.h"
#include "dlg_format.h"
#include "../resource.h"
#include "../settings.h"
#include "../string.h"
#include "../theme.h"

CEpisode Example;
CFormatWindow FormatWindow;
vector<wstring> functions, keywords;

// =============================================================================

CFormatWindow::CFormatWindow() {
  RegisterDlgClass(L"TaigaFormatW");
  Mode = 0;
}

BOOL CFormatWindow::OnInitDialog() {
  // Set up rich edit control
  m_RichEdit.Attach(GetDlgItem(IDC_RICHEDIT_FORMAT));
  m_RichEdit.SetEventMask(ENM_CHANGE);

  // Create temporary episode
  if (CurrentEpisode.AnimeId > 0) {
    Example = CurrentEpisode;
  } else {
    Example.AnimeId    = -4224;
    Example.AudioType  = L"AAC";
    Example.Checksum   = L"ABCD1234";
    Example.Extra      = L"DVD";
    Example.File       = L"[TaigaSubs]_Toradora!_-_01v2_-_Tiger_and_Dragon_[DVD][1280x720_H264_AAC][ABCD1234].mkv";
    Example.Folder     = L"D:\\Anime\\";
    Example.Format     = L"MKV";
    Example.Group      = L"TaigaSubs";
    Example.Name       = L"Tiger and Dragon";
    Example.Number     = L"01";
    Example.Resolution = L"1280x720";
    Example.Title      = L"Toradora!";
    Example.Version    = L"2";
    Example.VideoType  = L"H264";
  }

  // Set text
  switch (Mode) {
    case FORMAT_MODE_HTTP:
      SetText(L"Edit format - HTTP request");
      m_RichEdit.SetText(Settings.Announce.HTTP.Format.c_str());
      break;    
    case FORMAT_MODE_MESSENGER:
      SetText(L"Edit format - Messenger");
      m_RichEdit.SetText(Settings.Announce.MSN.Format.c_str());
      break;
    case FORMAT_MODE_MIRC:
      SetText(L"Edit format - mIRC");
      m_RichEdit.SetText(Settings.Announce.MIRC.Format.c_str());
      break;
    case FORMAT_MODE_SKYPE:
      SetText(L"Edit format - Skype");
      m_RichEdit.SetText(Settings.Announce.Skype.Format.c_str());
      break;
    case FORMAT_MODE_TWITTER:
      SetText(L"Edit format - Twitter");
      m_RichEdit.SetText(Settings.Announce.Twitter.Format.c_str());
      break;
    case FORMAT_MODE_BALLOON:
      SetText(L"Edit format - Balloon tooltips");
      m_RichEdit.SetText(Settings.Program.Balloon.Format.c_str());
      break;
  }

  return TRUE;
}

void CFormatWindow::OnOK() {
  switch (Mode) {
    case FORMAT_MODE_HTTP:
      m_RichEdit.GetText(Settings.Announce.HTTP.Format);
      break;
    case FORMAT_MODE_MESSENGER:
      m_RichEdit.GetText(Settings.Announce.MSN.Format);
      break;
    case FORMAT_MODE_MIRC:
      m_RichEdit.GetText(Settings.Announce.MIRC.Format);
      break;
    case FORMAT_MODE_SKYPE:
      m_RichEdit.GetText(Settings.Announce.Skype.Format);
      break;
    case FORMAT_MODE_TWITTER:
      m_RichEdit.GetText(Settings.Announce.Twitter.Format);
      break;
    case FORMAT_MODE_BALLOON:
      m_RichEdit.GetText(Settings.Program.Balloon.Format);
      break;
  }
  
  EndDialog(IDOK);
}

BOOL CFormatWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (LOWORD(wParam)) {
    // Add button    
    case IDHELP: {
      wstring answer = UI.Menus.Show(m_hWindow, 0, 0, L"ScriptAdd");
      wstring str; m_RichEdit.GetText(str);
      CHARRANGE cr = {0};
      m_RichEdit.GetSel(&cr);
      str.insert(cr.cpMin, answer);
      m_RichEdit.SetText(str.c_str());
      m_RichEdit.SetFocus();
      return TRUE;
    }
  }
  
  if (LOWORD(wParam) == IDC_RICHEDIT_FORMAT && HIWORD(wParam) == EN_CHANGE) {
    // Set preview text
    RefreshPreviewText();
    // Highlight
    ColorizeText();
    return TRUE;
  }
  return FALSE;
}

// =============================================================================

void CFormatWindow::ColorizeText() {
  // Save old selection
  CHARRANGE cr = {0};
  m_RichEdit.GetSel(&cr);
  m_RichEdit.SetRedraw(FALSE);
  m_RichEdit.HideSelection(TRUE);

  // Set up character format
  CHARFORMAT cf = {0};
  cf.cbSize = sizeof(CHARFORMAT);
  cf.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE;
  cf.yHeight = 200;
  lstrcpy(cf.szFaceName, L"Courier New");

  // Get current text
  wstring text; m_RichEdit.GetText(text);

  // Reset all colors
  cf.crTextColor = ::GetSysColor(COLOR_WINDOWTEXT);
  m_RichEdit.SetCharFormat(SCF_ALL, &cf);

  // Highlight
  for (size_t i = 0; i < text.length(); i++) {
    switch (text[i]) {
      // Highlight functions
      case '$': {
        cf.crTextColor = RGB(0, 0, 160);
        size_t pos = text.find('(', i);
        if (pos != wstring::npos) {
          if (IsScriptFunction(text.substr(i + 1, pos - (i + 1)))) {
            m_RichEdit.SetSel(i, pos);
            m_RichEdit.SetCharFormat(SCF_SELECTION, &cf);
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
            m_RichEdit.SetSel(i, pos + 1);
            m_RichEdit.SetCharFormat(SCF_SELECTION, &cf);
            i = pos;
          }
        }
        break;
      }
    }
  }

  // Reset to old selection
  m_RichEdit.SetSel(&cr);
  m_RichEdit.HideSelection(FALSE);
  m_RichEdit.SetRedraw(TRUE);
  m_RichEdit.InvalidateRect();
}

void CFormatWindow::RefreshPreviewText() {
  // Replace variables
  wstring str;
  GetDlgItemText(IDC_RICHEDIT_FORMAT, str);
  str = ReplaceVariables(str, Example);
  
  switch (Mode) {
    case FORMAT_MODE_MIRC: {
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
          if (IsNumeric(str[i])) str.erase(i, 1);
          if (IsNumeric(str[i])) str.erase(i, 1);
          i--; continue;
        }
      }
      break;
    }
    case FORMAT_MODE_SKYPE: {
      // Strip HTML codes
      StripHTML(str);
    }
  }
  
  // Set final text
  SetDlgItemText(IDC_EDIT_PREVIEW, str.c_str());
}