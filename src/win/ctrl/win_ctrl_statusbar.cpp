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

#include "win_ctrl.h"

namespace win {

StatusBar::StatusBar(HWND hwnd) {
  SetWindowHandle(hwnd);
}

void StatusBar::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = 0;
  cs.lpszClass = STATUSCLASSNAME;
  cs.style = WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | SBARS_TOOLTIPS;
}

void StatusBar::OnCreate(HWND hwnd, LPCREATESTRUCT create_struct) {
  widths_.clear();
  Window::OnCreate(hwnd, create_struct);
}

////////////////////////////////////////////////////////////////////////////////

void StatusBar::GetRect(int part, LPRECT rect) {
  SendMessage(SB_GETRECT, part, reinterpret_cast<LPARAM>(rect));
}

int StatusBar::InsertPart(int image, int style, int autosize, int width,
                          LPCWSTR text, LPCWSTR tooltip) {
  if (widths_.empty()) {
    widths_.push_back(width);
  } else {
    widths_.push_back(widths_.back() + width);
  }

  int part_count = SendMessage(SB_GETPARTS, 0, 0);

  SendMessage(SB_SETPARTS, part_count + 1,
      reinterpret_cast<LPARAM>(&widths_[0]));
  SendMessage(SB_SETTEXT, part_count - 1,
      reinterpret_cast<LPARAM>(text));
  SendMessage(SB_SETTIPTEXT, part_count - 1,
      reinterpret_cast<LPARAM>(tooltip));

  if (image > -1 && image_list_) {
    HICON icon = ::ImageList_GetIcon(image_list_, image, 0);
    SendMessage(SB_SETICON, part_count - 1, reinterpret_cast<LPARAM>(icon));
  }

  return part_count;
}

void StatusBar::SetImageList(HIMAGELIST image_list) {
  image_list_ = image_list;
}

void StatusBar::SetPartText(int part, LPCWSTR text) {
  SendMessage(SB_SETTEXT, part, reinterpret_cast<LPARAM>(text));
}

void StatusBar::SetPartTipText(int part, LPCWSTR tip_text) {
  SendMessage(SB_SETTIPTEXT, part, reinterpret_cast<LPARAM>(tip_text));
}

void StatusBar::SetPartWidth(int part, int width) {
  if (part > static_cast<int>(widths_.size()) - 1)
    return;

  if (part == 0) {
    widths_.at(part) = width;
  } else {
    widths_.at(part) = widths_.at(part - 1) + width;
  }

  SendMessage(SB_SETPARTS, widths_.size(),
              reinterpret_cast<LPARAM>(&widths_[0]));
}

}  // namespace win