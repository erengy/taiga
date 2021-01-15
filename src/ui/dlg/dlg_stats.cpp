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

#include "base/file.h"
#include "base/gfx.h"
#include "base/string.h"
#include "media/anime_db.h"
#include "sync/kitsu_util.h"
#include "sync/service.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "taiga/stats.h"
#include "ui/dlg/dlg_stats.h"
#include "ui/theme.h"
#include "ui/ui.h"

namespace ui {

StatsDialog DlgStats;

StatsDialog::StatsDialog()
    : win::Resizable(false, true) {
}

BOOL StatsDialog::OnInitDialog() {
  // Set new font for headers
  for (int i = 0; i < 4; i++) {
    SendDlgItemMessage(IDC_STATIC_HEADER1 + i, WM_SETFONT,
                       reinterpret_cast<WPARAM>(ui::Theme.GetBoldFont()), FALSE);
  }

  // Calculate and display statistics
  taiga::stats.CalculateAll();
  Refresh();

  return TRUE;
}

INT_PTR StatsDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  ResizeProc(hwnd, uMsg, wParam, lParam);

  switch (uMsg) {
    case WM_CTLCOLORSTATIC: {
      win::Dc dc = reinterpret_cast<HDC>(wParam);
      dc.SetBkMode(TRANSPARENT);
      dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
      dc.DetachDc();
      return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_WINDOW));
    }

    case WM_DRAWITEM: {
      // Draw score bars
      if (wParam == IDC_STATIC_ANIME_STAT2) {
        LPDRAWITEMSTRUCT dis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
        win::Rect rect = dis->rcItem;
        win::Dc dc = dis->hDC;

        dc.FillRect(dis->rcItem, ::GetSysColor(COLOR_WINDOW));

        int bar_height = GetTextHeight(dc.Get());
        int bar_max = rect.Width() * 3 / 4;

        for (int i = 10; i > 0; --i) {
          if (i < 10)
            rect.top += bar_height;

          if (taiga::stats.score_distribution[i] > 0.0f) {
            int bar_width = static_cast<int>(bar_max * taiga::stats.score_distribution[i]);
            rect.bottom = rect.top + bar_height - 2;
            rect.right = rect.left + bar_width;
            dc.FillRect(rect, ui::kColorDarkBlue);
          }

          if (taiga::stats.score_count[i] > 0.0f) {
            std::wstring text = ToWstr(taiga::stats.score_count[i]);
            win::Rect rect_text = rect;
            rect_text.left = rect_text.right += 8;
            rect_text.right = dis->rcItem.right;
            dc.EditFont(nullptr, 7);
            dc.SetBkMode(TRANSPARENT);
            dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
            dc.DrawText(text.c_str(), text.length(), rect_text,
                        DT_SINGLELINE | DT_VCENTER);
          }
        }

        dc.DetachDc();
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

  if (!lpps)
    return;

  // Paint background
  rect.Copy(lpps->rcPaint);
  dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));

  // Paint header lines
  for (int i = 0; i < 4; i++) {
    win::Rect rect_header;
    win::Window header = GetDlgItem(IDC_STATIC_HEADER1 + i);
    header.GetWindowRect(GetWindowHandle(), &rect_header);
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
      rect.Inflate(-ScaleX(kControlMargin) * 2, -ScaleY(kControlMargin));

      // Headers
      for (int i = 0; i < 4; i++) {
        win::Rect rect_header;
        win::Window header = GetDlgItem(IDC_STATIC_HEADER1 + i);
        header.GetWindowRect(GetWindowHandle(), &rect_header);
        rect_header.right = rect.right;
        header.SetPosition(nullptr, rect_header);
        header.SetWindowHandle(nullptr);
      }

      // Redraw
      InvalidateRect();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void StatsDialog::Refresh() {
  if (!IsWindow())
    return;

  enum class RatingType {
    Five,
    Ten,
  } rating_type = RatingType::Ten;
  switch (sync::GetCurrentServiceId()) {
    case sync::ServiceId::Kitsu:
      switch (sync::kitsu::GetRatingSystem()) {
        case sync::kitsu::RatingSystem::Regular:
          rating_type = RatingType::Five;
          break;
      }
      break;
  }

  // Anime list
  std::wstring text;
  text += ToWstr(taiga::stats.anime_count) + L"\n";
  text += ToWstr(taiga::stats.episode_count) + L"\n";
  text += taiga::stats.life_spent_watching + L"\n";
  text += taiga::stats.life_planned_to_watch + L"\n";
  switch (rating_type) {
    case RatingType::Ten:
      text += ToWstr(taiga::stats.score_mean / 10.0, 2) + L"\n";
      text += ToWstr(taiga::stats.score_deviation / 10.0, 2);
      break;
    case RatingType::Five:
      text += ToWstr(taiga::stats.score_mean / 20.0, 2) + L"\n";
      text += ToWstr(taiga::stats.score_deviation / 20.0, 2);
      break;
  }
  SetDlgItemText(IDC_STATIC_ANIME_STAT1, text.c_str());

  // Score distribution
  text.clear();
  switch (rating_type) {
    case RatingType::Ten:
      text = L"10\n9\n8\n7\n6\n5\n4\n3\n2\n1\n";
      break;
    case RatingType::Five:
      text = L"5.0\n4.5\n4.0\n3.5\n3.0\n2.5\n2.0\n1.5\n1.0\n0.5";
      break;
  }
  SetDlgItemText(IDC_STATIC_ANIME_STAT2_LABEL, text.c_str());
  win::Window window = GetDlgItem(IDC_STATIC_ANIME_STAT2);
  win::Rect rect;
  window.GetWindowRect(GetWindowHandle(), &rect);
  InvalidateRect(&rect);
  window.SetWindowHandle(nullptr);

  // Database
  text.clear();
  text += ToWstr(anime::db.items.size()) + L"\n";
  text += ToWstr(taiga::stats.image_count) + L" (" + ToSizeString(taiga::stats.image_size) + L")\n";
  text += ToWstr(taiga::stats.torrent_count) + L" (" + ToSizeString(taiga::stats.torrent_size) + L")";
  SetDlgItemText(IDC_STATIC_ANIME_STAT3, text.c_str());

  // Taiga
  text.clear();
  text += ToWstr(taiga::stats.connections_succeeded + taiga::stats.connections_failed);
  if (taiga::stats.connections_failed > 0)
    text += L" (" + ToWstr(taiga::stats.connections_failed) + L" failed)";
  text += L"\n";
  text += ToDateString(taiga::stats.uptime) + L"\n";
  text += ToWstr(taiga::stats.tigers_harmed);
  SetDlgItemText(IDC_STATIC_ANIME_STAT4, text.c_str());
}

}  // namespace ui
