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

#include "base/std.h"
#include <cmath>

#include "dlg_about.h"

#include "base/common.h"
#include "base/file.h"
#include "base/gfx.h"
#include "taiga/resource.h"
#include "taiga/stats.h"
#include "base/string.h"
#include "taiga/taiga.h"
#include "base/time.h"

#include "win/win_gdi.h"

class AboutDialog AboutDialog;

// =============================================================================

#define NOTE_COUNT 32

int note_index;
int note_list[NOTE_COUNT][2] = {
  {84, 1}, // 1/2
  {84, 2}, // 1/4
  {86, 4}, // 1/8
  {84, 2}, // 1/4
  {82, 2}, // 1/4
  {81, 2}, // 1/4
  {77, 4}, // 1/8
  {79, 4}, // 1/8
  {72, 4}, // 1/8
  {77, 1}, // 1/2
  {76, 4}, // 1/8
  {77, 4}, // 1/8
  {79, 4}, // 1/8
  {81, 2}, // 1/4
  {79, 2}, // 1/4
  {77, 2}, // 1/4
  {79, 2}, // 1/4
  {81, 4}, // 1/8
  {84, 1}, // 1/2
  {84, 2}, // 1/2
  {86, 4}, // 1/8
  {84, 2}, // 1/4
  {82, 2}, // 1/4
  {81, 2}, // 1/4
  {77, 4}, // 1/8
  {79, 4}, // 1/8
  {72, 4}, // 1/8
  {77, 1}, // 1/2
  {76, 4}, // 1/8
  {77, 4}, // 1/8
  {76, 4}, // 1/8
  {74, 1}  // 1/1
};

float NoteToFrequency(int n) {
  if (n < 0 || n > 119) return -1.0f;
  return static_cast<float>(440.0 * pow(2.0, static_cast<double>(n - 57) / 12.0));
}

// =============================================================================

AboutDialog::AboutDialog() {
  RegisterDlgClass(L"TaigaAboutW");
}

BOOL AboutDialog::OnDestroy() {
  KillTimer(GetWindowHandle(), TIMER_TAIGA);
  return TRUE;
}

BOOL AboutDialog::OnInitDialog() {
  rich_edit_.Attach(GetDlgItem(IDC_RICHEDIT_ABOUT));
  rich_edit_.SendMessage(EM_AUTOURLDETECT, TRUE /* AURL_ENABLEURL */);
  rich_edit_.SetEventMask(ENM_LINK);

  wstring text = 
    L"{\\rtf1\\ansi\\deff0"
    L"{\\fonttbl"
    L"{\\f0 Segoe UI;}"
    L"}"
    L"\\deflang1024\\fs18"
    L"\\b " TAIGA_APP_NAME L"\\b0\\line "
    L"version " + std::wstring(Taiga.version) + L"\\line\\line "
    L"\\b Author:\\b0\\line "
    L"Eren 'erengy' Okka\\line\\line "
    L"\\b Committers and other contributors:\\b0\\line "
    L"saka, Diablofan, slevir, LordGravewish\\line\\line "
    L"\\b Third party stuff that is used by Taiga:\\b0\\line "
    L"- Fugue Icons 3.4.5, Copyright (c) 2012, Yusuke Kamiyamane\\line "
    L"- JsonCpp 0.6.0, Copyright (c) 2007-2010, Baptiste Lepilleur\\line "
    L"- OAuth class is based on codebrook-twitter-oauth example code, Copyright (c) 2010, Brook Miles\\line "
    L"- pugixml parser version 1.2, Copyright (c) 2006-2012, Arseny Kapoulkine\\line "
    L"- zlib version 1.2.5, Copyright (c) 1995-2010, Jean-loup Gailly and Mark Adler\\line\\line "
    L"\\b Links:\\b0\\line "
    L"- Home page {\\field{\\*\\fldinst HYPERLINK \"http://taiga.erengy.com\"}{\\fldrslt http://taiga.erengy.com}}\\line "
    L"- Google Code {\\field{\\*\\fldinst HYPERLINK \"http://code.google.com/p/taiga/\"}{\\fldrslt http://code.google.com/p/taiga/}}\\line "
    L"- MyAnimeList club {\\field{\\*\\fldinst HYPERLINK \"http://myanimelist.net/clubs.php?cid=21400\"}{\\fldrslt http://myanimelist.net/clubs.php?cid=21400}}\\line "
    L"- IRC channel {\\field{\\*\\fldinst HYPERLINK \"irc://irc.rizon.net/taiga\"}{\\fldrslt irc://irc.rizon.net/taiga}}"
    L"}";
  rich_edit_.SetTextEx(WstrToStr(text));

  return TRUE;
}

BOOL AboutDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_COMMAND: {
      // Icon click
      if (HIWORD(wParam) == STN_DBLCLK) {
        Stats.tigers_harmed++;
        SetTimer(hwnd, TIMER_TAIGA, 100, nullptr);
        note_index = 0;
        return TRUE;
      }
      break;
    }

    case WM_NOTIFY: {
      switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
        // Execute link
        case EN_LINK: {
          auto en_link = reinterpret_cast<ENLINK*>(lParam);
          if (en_link->msg == WM_LBUTTONUP) {
            ExecuteLink(rich_edit_.GetTextRange(&en_link->chrg));
            return TRUE;
          }
          break;
        }
      }
      break;
    }
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

void AboutDialog::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  win::Dc dc = hdc;
  win::Rect rect;

  // Paint background
  GetClientRect(&rect);
  rect.left = ScaleX(static_cast<int>(48 * 1.5f));
  dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));
}

void AboutDialog::OnTimer(UINT_PTR nIDEvent) {
  if (note_index == NOTE_COUNT) {
    KillTimer(GetWindowHandle(), TIMER_TAIGA);
    SetText(L"About");
    note_index = 0;
  } else {
    if (note_index == 0) SetText(L"Orange");
    Beep((DWORD)NoteToFrequency(note_list[note_index][0]), 800 / note_list[note_index][1]);
    note_index++;
  }
}