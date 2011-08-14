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
#include <math.h>
#include "../common.h"
#include "dlg_about.h"
#include "../resource.h"
#include "../taiga.h"
#include "../win32/win_gdi.h"

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
  // Initialize
  tab_.Attach(GetDlgItem(IDC_TAB_ABOUT));

  // Create about page
  page_taiga_.Create(IDD_ABOUT_TAIGA, m_hWindow, false);
  CRect rect; tab_.AdjustRect(m_hWindow, FALSE, &rect);
  page_taiga_.SetPosition(nullptr, rect, 0);
  EnableThemeDialogTexture(page_taiga_.GetWindowHandle(), ETDT_ENABLETAB);

  return TRUE;
}

// =============================================================================

BOOL AboutPage::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Icon click
    case WM_COMMAND: {
      if (HIWORD(wParam) == STN_DBLCLK) {
        SetTimer(hwnd, TIMER_TAIGA, 100, nullptr);
        note_index = 0;
        return TRUE;
      }
      break;
    }

    // Execute link
    case WM_NOTIFY: {
      switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
        case NM_CLICK:
        case NM_RETURN: {
          PNMLINK pNMLink = reinterpret_cast<PNMLINK>(lParam);
          ExecuteAction(pNMLink->item.szUrl);
          return TRUE;
        }
      }
      break;
    }
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

BOOL AboutPage::OnInitDialog() {
  // Set information text
  wstring text = APP_NAME;
  SetDlgItemText(IDC_STATIC_ABOUT_TITLE, text.c_str());
  text = L"Version "; text += APP_VERSION;
  SetDlgItemText(IDC_STATIC_ABOUT_VERSION, text.c_str());
  text = L"Built on "; text += APP_BUILD;
  SetDlgItemText(IDC_STATIC_ABOUT_BUILD, text.c_str());

  return TRUE;
}

void AboutPage::OnTimer(UINT_PTR nIDEvent) {
  if (note_index == NOTE_COUNT) {
    KillTimer(GetWindowHandle(), TIMER_TAIGA);
    AboutDialog.SetText(L"About");
    note_index = 0;
  } else {
    if (note_index == 0) AboutDialog.SetText(L"Orange");
    Beep((DWORD)NoteToFrequency(note_list[note_index][0]), 800 / note_list[note_index][1]);
    note_index++;
  }
}