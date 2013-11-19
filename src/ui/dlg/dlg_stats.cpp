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

#include "base/std.h"

#include "dlg_stats.h"

#include "library/anime_db.h"
#include "base/common.h"
#include "base/gfx.h"
#include "sync/myanimelist.h"
#include "taiga/resource.h"
#include "taiga/stats.h"
#include "base/string.h"
#include "ui/theme.h"

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
  ResizeProc(hwnd, uMsg, wParam, lParam);

  switch (uMsg) {
    case WM_CTLCOLORSTATIC:
      return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_WINDOW));

    case WM_DRAWITEM: {
      // Draw score bars
      if (wParam == IDC_STATIC_ANIME_STAT2) {
        LPDRAWITEMSTRUCT dis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
        win::Rect rect = dis->rcItem;
        win::Dc dc = dis->hDC;

        dc.FillRect(dis->rcItem, ::GetSysColor(COLOR_WINDOW));
        
        int bar_height = GetTextHeight(dc.Get());
        int bar_max = rect.Width() * 3 / 4;
        
        for (int i = 10; i > 0; i--) {
          if (i < 10)
            rect.top += bar_height;

          if (Stats.score_distribution[i] > 0.0f) {
            int bar_width = static_cast<int>(bar_max * Stats.score_distribution[i]);
            rect.bottom = rect.top + bar_height - 2;
            rect.right = rect.left + bar_width;
            dc.FillRect(rect, theme::COLOR_DARKBLUE);
          }

          if (Stats.score_count[i] > 0.0f) {
            wstring text = ToWstr(Stats.score_count[i]);
            win::Rect rect_text = rect;
            rect_text.left = rect_text.right += 8;
            rect_text.right = dis->rcItem.right;
            dc.EditFont(nullptr, 7);
            dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
            dc.DrawText(text.c_str(), text.length(), rect_text,
                        DT_SINGLELINE | DT_VCENTER);
          }
        }
        
        dc.DetachDC();
        return TRUE;
      }
      break;
    }

    case WM_ERASEBKGND:
      return TRUE;
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

void StatsDialog::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  win::Dc dc = hdc;
  win::Rect rect;

  // Paint background
  rect.Copy(lpps->rcPaint);
  dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));

  // Paint header lines
  for (int i = 0; i < 4; i++) {
    win::Rect rect_header;
    win::Window header = GetDlgItem(IDC_STATIC_HEADER1 + i);
    header.GetWindowRect(m_hWindow, &rect_header);
    rect_header.top = rect_header.bottom + 3;
    rect_header.bottom =  rect_header.top + 1;
    dc.FillRect(rect_header, ::GetSysColor(COLOR_ACTIVEBORDER));
    rect_header.Offset(0, 1);
    dc.FillRect(rect_header, ::GetSysColor(COLOR_WINDOW));
    header.SetWindowHandle(nullptr);
  }
}

void StatsDialog::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_SIZE: {
      win::Rect rect;
      rect.Set(0, 0, size.cx, size.cy);
      rect.Inflate(-ScaleX(WIN_CONTROL_MARGIN) * 2, -ScaleY(WIN_CONTROL_MARGIN));

      // Headers
      for (int i = 0; i < 4; i++) {
        win::Rect rect_header;
        win::Window header = GetDlgItem(IDC_STATIC_HEADER1 + i);
        header.GetWindowRect(m_hWindow, &rect_header);
        rect_header.right = rect.right;
        header.SetPosition(nullptr, rect_header);
        header.SetWindowHandle(nullptr);
      }

      // Redraw
      InvalidateRect();
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
  win::Window window = GetDlgItem(IDC_STATIC_ANIME_STAT2);
  win::Rect rect;
  window.GetWindowRect(GetWindowHandle(), &rect);
  InvalidateRect(&rect);
  window.SetWindowHandle(nullptr);

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