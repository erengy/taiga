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

#ifndef TAIGA_WIN_MENU_H
#define TAIGA_WIN_MENU_H

#include "win_main.h"

namespace win {

enum MenuItemType {
  kMenuItemDefault = 0,
  kMenuItemSeparator,
  kMenuItemSubmenu
};

class MenuItem {
public:
  bool checked, def, enabled, new_column, radio;
  std::wstring action, name, submenu;
  int type;
};

class Menu {
public:
  void CreateItem(std::wstring action = L"",
                  std::wstring name = L"",
                  std::wstring submenu = L"",
                  bool checked = false,
                  bool def = false,
                  bool enabled = true,
                  bool newcolumn = false,
                  bool radio = false);

  std::vector<MenuItem> items;
  std::wstring name;
  std::wstring type;
};

class MenuList {
public:
  void Create(LPCWSTR name, LPCWSTR type);
  HMENU CreateNewMenu(LPCWSTR name, std::vector<HMENU>& menu_handles);
  Menu* FindMenu(LPCWSTR name);
  std::wstring Show(HWND hwnd, int x, int y, LPCWSTR name);

  std::vector<Menu> menus;
};

}  // namespace win

#endif  // TAIGA_WIN_MENU_H