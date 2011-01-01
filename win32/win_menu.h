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

#ifndef WIN_MENU_H
#define WIN_MENU_H

#include "win_main.h"

// =============================================================================

enum MenuItemType {
  MENU_ITEM_NORMAL = 0,
  MENU_ITEM_SEPERATOR,
  MENU_ITEM_SUBMENU
};

class CMenuList {
public:
  CMenuList() : m_hImageList(NULL) {}
  ~CMenuList() {}

  void    Create(LPCWSTR lpName, LPCWSTR lpType);
  HMENU   CreateNewMenu(LPCWSTR lpName, vector<HMENU>& hMenu);
  int     GetIndex(LPCWSTR lpName);
  void    SetImageList(HIMAGELIST hImageList);
  wstring Show(HWND hwnd, int x, int y, LPCWSTR lpName);

  class CMenu {
  public:
    void CreateItem(
      wstring action = L"", 
      wstring name   = L"", 
      wstring sub    = L"",
      bool checked   = false, 
      bool def       = false, 
      bool enabled   = true, 
      bool newcolumn = false, 
      bool radio     = false);

    class CMenuItem {
    public:
      bool    Checked, Default, Enabled, NewColumn, Radio;
      wstring Action, Name, SubMenu;
      int     Type;

      class CMenuIcon {
      public:
        void    Destroy();
        void    Load(HICON hIcon);
        HBITMAP Handle;
        int     Index;
      } Icon;
    };
    vector<CMenuItem> Item;
    wstring Name, Type;
  };
  vector<CMenu> Menu;

private:
  HIMAGELIST m_hImageList;
};

#endif // WIN_MENU_H