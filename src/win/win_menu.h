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

#ifndef WIN_MENU_H
#define WIN_MENU_H

#include "win_main.h"

namespace win {

// =============================================================================

enum MenuItemType {
  MENU_ITEM_NORMAL = 0,
  MENU_ITEM_SEPARATOR,
  MENU_ITEM_SUBMENU
};

class MenuItem {
public:
  bool     Checked, Default, Enabled, NewColumn, Radio;
  wstring  Action, Name, SubMenu;
  int      Type;
};

class Menu {
public:
  void CreateItem(wstring action = L"",
                  wstring name = L"",
                  wstring sub = L"",
                  bool checked = false,
                  bool def = false,
                  bool enabled = true,
                  bool newcolumn = false,
                  bool radio = false);

  vector<MenuItem> items;
  wstring name;
  wstring type;
};

class MenuList {
public:
  void    Create(LPCWSTR lpName, LPCWSTR lpType);
  HMENU   CreateNewMenu(LPCWSTR lpName, vector<HMENU>& hMenu);
  Menu*   FindMenu(LPCWSTR lpName);
  wstring Show(HWND hwnd, int x, int y, LPCWSTR lpName);

  vector<Menu> menus;
};

}  // namespace win

#endif // WIN_MENU_H