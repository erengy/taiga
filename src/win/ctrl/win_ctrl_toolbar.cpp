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

Toolbar::Toolbar(HWND hwnd) {
  SetWindowHandle(hwnd);
}

void Toolbar::PreCreate(CREATESTRUCT &cs) {
  cs.dwExStyle = 0;
  cs.lpszClass = TOOLBARCLASSNAME;
  cs.style = WS_CHILD | WS_VISIBLE | WS_TABSTOP |
             CCS_NODIVIDER | CCS_NOPARENTALIGN |
             TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS |
             TBSTYLE_TRANSPARENT;
}

void Toolbar::OnCreate(HWND hwnd, LPCREATESTRUCT create_struct) {
  SendMessage(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
  SendMessage(TB_SETEXTENDEDSTYLE, 0,
              TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
  SetImageList(nullptr, 0, 0);
  Window::OnCreate(hwnd, create_struct);
}

////////////////////////////////////////////////////////////////////////////////

BOOL Toolbar::EnableButton(int command_id, bool enabled) {
  TBBUTTONINFO tbbi = {0};
  tbbi.cbSize = sizeof(TBBUTTONINFO);
  tbbi.dwMask = TBIF_STATE;

  SendMessage(TB_GETBUTTONINFO, command_id, reinterpret_cast<LPARAM>(&tbbi));

  if (enabled) {
    tbbi.fsState |= TBSTATE_ENABLED;
    tbbi.fsState &= ~TBSTATE_INDETERMINATE;
  } else {
    tbbi.fsState |= TBSTATE_INDETERMINATE;
    tbbi.fsState &= ~TBSTATE_ENABLED;
  }

  return SendMessage(TB_SETBUTTONINFO, command_id,
                     reinterpret_cast<LPARAM>(&tbbi));
}

int Toolbar::GetHeight() {
  RECT rect;
  SendMessage(TB_GETITEMRECT, 1, reinterpret_cast<LPARAM>(&rect));
  return rect.bottom;
}

BOOL Toolbar::GetButton(int index, TBBUTTON& tbb) {
  return SendMessage(TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&tbb));
}

int Toolbar::GetButtonCount() {
  return SendMessage(TB_BUTTONCOUNT, 0, 0);
}

int Toolbar::GetButtonIndex(int command_id) {
  TBBUTTONINFO tbbi = {0};
  tbbi.cbSize = sizeof(tbbi);
  tbbi.dwMask = TBIF_COMMAND;
  tbbi.idCommand = command_id;

  return SendMessage(TB_GETBUTTONINFO, command_id,
                     reinterpret_cast<LPARAM>(&tbbi));
}

DWORD Toolbar::GetButtonSize() {
  return SendMessage(TB_GETBUTTONSIZE, 0, 0);
}

DWORD Toolbar::GetButtonStyle(int index) {
  TBBUTTON tbb = {0};
  SendMessage(TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&tbb));
  return tbb.fsStyle;
}

LPCWSTR Toolbar::GetButtonTooltip(int index) {
  if (index < static_cast<int>(tooltip_text_.size()))
    if (!tooltip_text_[index].empty())
      return tooltip_text_[index].c_str();

  return nullptr;
}

int Toolbar::GetHotItem() {
  return SendMessage(TB_GETHOTITEM, 0, 0);
}

DWORD Toolbar::GetPadding() {
  return SendMessage(TB_GETPADDING, 0, 0);
}

int Toolbar::HitTest(POINT& point) {
  return SendMessage(TB_HITTEST, 0, reinterpret_cast<LPARAM>(&point));
}

BOOL Toolbar::InsertButton(int index, int bitmap, int command_id,
                           BYTE state, BYTE style, DWORD_PTR data,
                           LPCWSTR text, LPCWSTR tooltip) {
  TBBUTTON tbb = {0};
  tbb.iBitmap = bitmap;
  tbb.idCommand = command_id;
  tbb.iString = reinterpret_cast<INT_PTR>(text);
  tbb.fsState = state;
  tbb.fsStyle = style;
  tbb.dwData = data;

  tooltip_text_.push_back(tooltip ? tooltip : L"");
  return SendMessage(TB_INSERTBUTTON, index, reinterpret_cast<LPARAM>(&tbb));
}

BOOL Toolbar::MapAcelerator(UINT character, UINT& command_id) {
  return SendMessage(TB_MAPACCELERATOR, character,
                     reinterpret_cast<LPARAM>(&command_id));
}

BOOL Toolbar::PressButton(int command_id, BOOL press) {
  return SendMessage(TB_PRESSBUTTON, command_id, MAKELPARAM(press, 0));
}

BOOL Toolbar::SetButtonImage(int index, int image) {
  TBBUTTONINFO tbbi = {0};
  tbbi.cbSize = sizeof(TBBUTTONINFO);
  tbbi.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
  tbbi.iImage = image;

  return SendMessage(TB_SETBUTTONINFO, index, reinterpret_cast<LPARAM>(&tbbi));
}

BOOL Toolbar::SetButtonText(int index, LPCWSTR text) {
  TBBUTTONINFO tbbi = {0};
  tbbi.cbSize = sizeof(TBBUTTONINFO);
  tbbi.dwMask = TBIF_BYINDEX | TBIF_TEXT;
  tbbi.pszText = const_cast<LPWSTR>(text);

  return SendMessage(TB_SETBUTTONINFO, index, reinterpret_cast<LPARAM>(&tbbi));
}

BOOL Toolbar::SetButtonTooltip(int index, LPCWSTR tooltip) {
  if (index > static_cast<int>(tooltip_text_.size())) {
    return FALSE;
  } else {
    tooltip_text_[index] = tooltip;
    return TRUE;
  }
}

int Toolbar::SetHotItem(int index) {
  return SendMessage(TB_SETHOTITEM, index, 0);
}

int Toolbar::SetHotItem(int index, DWORD flags) {
  return SendMessage(TB_SETHOTITEM2, index, flags);
}

void Toolbar::SetImageList(HIMAGELIST image_list, int dx, int dy) {
  SendMessage(TB_SETBITMAPSIZE, 0, MAKELONG(dx, dy));
  SendMessage(TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(image_list));
}

}  // namespace win