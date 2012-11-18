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

#include "dlg_stats.h"

#include "../anime_db.h"
#include "../common.h"
#include "../gfx.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../stats.h"
#include "../string.h"
#include "../theme.h"

class StatsDialog StatsDialog;

// =============================================================================

BOOL StatsDialog::OnInitDialog() {
  // Set new font for headers
  for (int i = 0; i < 4; i++) {
    SendDlgItemMessage(IDC_STATIC_HEADER1 + i, WM_SETFONT, 
      reinterpret_cast<WPARAM>(UI.font_bold.Get()), FALSE);
  }

  // Calculate and display statistics
  Stats.CalculateAll();
  Refresh();

  return TRUE;
}

INT_PTR StatsDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CTLCOLORSTATIC:
      return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));

    case WM_DRAWITEM: {
      // Draw score bars
      if (wParam == IDC_STATIC_ANIME_STAT2) {
        LPDRAWITEMSTRUCT dis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
        win32::Rect rect = dis->rcItem;
        win32::Dc dc = dis->hDC;

        dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));
        
        int bar_height = GetTextHeight(dc.Get());
        int bar_left = rect.left;
        int bar_max = rect.Width() * 3 / 4;
        
        for (int i = 10; i > 0; i--) {
          int bar_width = static_cast<int>(bar_max * Stats.score_distribution[i]);
          if (i < 10) rect.top += bar_height;
          rect.bottom = rect.top + bar_height - 2;
          rect.right = rect.left + bar_width;
          dc.FillRect(rect, mal::COLOR_DARKBLUE);
        }
        
        dc.DetachDC();
        return TRUE;
      }
      break;
    }
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

void StatsDialog::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  win32::Dc dc = hdc;
  win32::Rect rect;

  // Paint background
  rect.Copy(lpps->rcPaint);
  dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));
  rect.right = 1;
  dc.FillRect(rect, ::GetSysColor(COLOR_ACTIVEBORDER));
}

void StatsDialog::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_SIZE: {
      win32::Rect rcWindow;
      rcWindow.Set(0, 0, size.cx, size.cy);
      // TODO: Resize horizontally
    }
  }
}

// =============================================================================

void StatsDialog::Refresh() {
  if (!IsWindow()) return;
  
  // Anime list
  wstring text;
  text += ToWstr(Stats.anime_count) + L"\n";
  text += ToWstr(Stats.episode_count) + L"\n";
  text += Stats.life_spent_watching + L"\n";
  text += ToWstr(Stats.score_mean, 2) + L"\n";
  text += ToWstr(Stats.score_deviation, 2);
  SetDlgItemText(IDC_STATIC_ANIME_STAT1, text.c_str());

  // Score distribution
  RedrawWindow();

  // Database
  text.clear();
  text += ToWstr(static_cast<int>(AnimeDatabase.items.size())) + L"\n";
  text += ToWstr(Stats.image_count) + L" (" + ToSizeString(Stats.image_size) + L")\n";
  text += ToWstr(Stats.torrent_count) + L" (" + ToSizeString(Stats.torrent_size) + L")";
  SetDlgItemText(IDC_STATIC_ANIME_STAT3, text.c_str());

  // Taiga
  text.clear();
  text += ToWstr(Stats.connections_succeeded + Stats.connections_failed);
  if (Stats.connections_failed > 0)
    text += L" (" + ToWstr(Stats.connections_failed) + L" failed)";
  text += L"\n";
  text += ToDateString(Stats.uptime) + L"\n";
  text += ToWstr(Stats.tigers_harmed);
  SetDlgItemText(IDC_STATIC_ANIME_STAT4, text.c_str());
}