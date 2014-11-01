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

#include "win_main.h"
#include "win_resizable.h"
#include "win_gdi.h"

// see: http://support.microsoft.com/kb/262954

namespace win {

Resizable::Resizable()
    : horizontal_(true), vertical_(true),
      x_(1), y_(1) {
}

Resizable::Resizable(bool horizontal, bool vertical)
    : horizontal_(horizontal), vertical_(vertical),
      x_(1), y_(1) {
}

void Resizable::ResizeProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_INITDIALOG:
      OnInitDialog(hwnd);
      break;
    case WM_HSCROLL:
      OnScroll(hwnd, SB_HORZ, LOWORD(wParam));
      break;
    case WM_VSCROLL:
      OnScroll(hwnd, SB_VERT, LOWORD(wParam));
      break;
    case WM_SIZE: {
      SIZE size = {LOWORD(lParam), HIWORD(lParam)};
      OnSize(hwnd, static_cast<UINT>(wParam), size);
      break;
    }
  }
}

void Resizable::OnInitDialog(HWND hwnd) {
  Rect rect;
  ::GetClientRect(hwnd, &rect);

  SCROLLINFO si = {0};
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
  si.nMin = 1;
  si.nPos = 1;

  if (horizontal_) {
    si.nMax = rect.Width();
    si.nPage = rect.Width();
    ::SetScrollInfo(hwnd, SB_HORZ, &si, FALSE);
  }
  if (vertical_) {
    si.nMax = rect.Height();
    si.nPage = rect.Height();
    ::SetScrollInfo(hwnd, SB_VERT, &si, FALSE);
  }
}

void Resizable::OnScroll(HWND hwnd, int bar, UINT code) {
  SCROLLINFO si = {0};
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
  ::GetScrollInfo(hwnd, bar, &si);

  int min_pos = si.nMin;
  int max_pos = si.nMax - (si.nPage - 1);
  int pos = -1;

  switch (code) {
    case SB_LINEUP:
      pos = max(si.nPos - 1, min_pos);
      break;
    case SB_LINEDOWN:
      pos = min(si.nPos + 1, max_pos);
      break;
    case SB_PAGEUP:
      pos = max(si.nPos - (int)si.nPage, min_pos);
      break;
    case SB_PAGEDOWN:
      pos = min(si.nPos + (int)si.nPage, max_pos);
      break;
    case SB_THUMBPOSITION:
      break;
    case SB_THUMBTRACK:
      pos = si.nTrackPos;
      break;
    case SB_TOP:
      pos = min_pos;
      break;
    case SB_BOTTOM:
      pos = max_pos;
      break;
    case SB_ENDSCROLL:
      break;
    default:
      return;
  }

  if (pos == -1)
    return;

  ::SetScrollPos(hwnd, bar, pos, TRUE);
  ScrollClient(hwnd, bar, pos);
}

void Resizable::OnSize(HWND hwnd, UINT type, SIZE size) {
  if (type != SIZE_RESTORED && type != SIZE_MAXIMIZED)
    return;

  SCROLLINFO si = {0};
  si.cbSize = sizeof(SCROLLINFO);

  const int bar[] = { SB_HORZ, SB_VERT };
  const int page[] = { size.cx, size.cy };

  for (size_t i = 0; i < ARRAYSIZE(bar); i++) {
    si.fMask = SIF_PAGE;
    si.nPage = page[i];
    ::SetScrollInfo(hwnd, bar[i], &si, TRUE);

    si.fMask = SIF_RANGE | SIF_POS;
    ::GetScrollInfo(hwnd, bar[i], &si);

    const int max_pos = si.nMax - (page[i] - 1);

    if ((si.nPos != si.nMin && si.nPos == max_pos) ||
        (type == SIZE_MAXIMIZED))
      ScrollClient(hwnd, bar[i], si.nPos);
  }
}

void Resizable::ScrollClient(HWND hwnd, int bar, int pos) {
  int x = 0;
  int y = 0;

  int& delta = bar == SB_HORZ ? x : y;
  int& prev = bar == SB_HORZ ? x_ : y_;

  delta = prev - pos;
  prev = pos;

  if (x || y)
    ::ScrollWindow(hwnd, x, y, nullptr, nullptr);
}

}  // namespace win