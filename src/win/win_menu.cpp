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

#include "win_menu.h"

namespace win {

// =============================================================================

HMENU MenuList::CreateNewMenu(LPCWSTR lpName, vector<HMENU>& hMenu) {
  // Initialize
  auto menu = FindMenu(lpName);
  if (!menu)
    return NULL;
  int nMenu = hMenu.size();
  hMenu.resize(nMenu + 1);

  // Create menu
  if (menu->type == L"menubar") {
    hMenu[nMenu] = ::CreateMenu();
  } else {
    hMenu[nMenu] = ::CreatePopupMenu();
  }
  if (!hMenu[nMenu])
    return NULL;

  // Add items
  for (auto item = menu->items.begin(); item != menu->items.end(); ++item) {
    UINT uFlags = (item->Checked ? MF_CHECKED : NULL) |
                  (item->Default ? MF_DEFAULT : NULL) |
                  (item->Enabled ? MF_ENABLED : MF_GRAYED) |
                  (item->NewColumn ? MF_MENUBARBREAK : NULL) |
                  (item->Radio ? MFT_RADIOCHECK : NULL);
    
    switch (item->Type) {
      // Normal item      
      case MENU_ITEM_NORMAL: {
        UINT_PTR uIDNewItem = (UINT_PTR)&item->Action;
        ::AppendMenu(hMenu[nMenu], MF_STRING | uFlags, uIDNewItem,
                     item->Name.c_str());
        if (item->Default) {
          ::SetMenuDefaultItem(hMenu[nMenu], uIDNewItem, FALSE);
        }
        break;
      }
      // Separator
      case MENU_ITEM_SEPARATOR: {
        ::AppendMenu(hMenu[nMenu], MF_SEPARATOR, NULL, NULL);
        break;
      }
      // Sub menu
      case MENU_ITEM_SUBMENU: {
        auto submenu = FindMenu(item->SubMenu.c_str());
        if (submenu && submenu != menu) {
          HMENU hSubMenu = CreateNewMenu(submenu->name.c_str(), hMenu);
          ::AppendMenu(hMenu[nMenu], MF_POPUP | uFlags, 
                       (UINT_PTR)hSubMenu, item->Name.c_str());
        }
        break;
      }
    }
  }

  return hMenu[nMenu];
}

wstring MenuList::Show(HWND hwnd, int x, int y, LPCWSTR lpName) {
  // Create menu
  vector<HMENU> hMenu;
  CreateNewMenu(lpName, hMenu);
  if (!hMenu.size())
    return L"";

  // Set position
  if (x == 0 && y == 0) {
    POINT point;
    ::GetCursorPos(&point);
    x = point.x;
    y = point.y;
  }
  
  // Show menu
  UINT_PTR index = ::TrackPopupMenuEx(hMenu[0], 
    TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD, 
    x, y, hwnd, NULL);

  // Clean-up
  for (unsigned int i = 0; i < hMenu.size(); i++) {
    ::DestroyMenu(hMenu[i]);
  }

  // Return action
  if (index > 0) {
    wstring* str = reinterpret_cast<wstring*>(index);
    return *str;
  } else {
    return L"";
  }
}

void MenuList::Create(LPCWSTR lpName, LPCWSTR lpType) {
  menus.resize(menus.size() + 1);
  menus.back().name = lpName;
  menus.back().type = lpType;
}

Menu* MenuList::FindMenu(LPCWSTR lpName) {
  for (auto it = menus.begin(); it != menus.end(); ++it)
    if (it->name == lpName)
      return &(*it);

  return nullptr;
}

// =============================================================================

void Menu::CreateItem(wstring action, wstring name, wstring sub,
                      bool checked, bool def, bool enabled,
                      bool newcolumn, bool radio) {
  unsigned int i = items.size();
  items.resize(i + 1);
  
  items[i].Action    = action;
  items[i].Checked   = checked;
  items[i].Default   = def;
  items[i].Enabled   = enabled;
  items[i].Name      = name;
  items[i].NewColumn = newcolumn;
  items[i].Radio     = radio;
  items[i].SubMenu   = sub;

  if (!sub.empty()) {
    items[i].Type = MENU_ITEM_SUBMENU;
  } else if (name.empty()) {
    items[i].Type = MENU_ITEM_SEPARATOR;
  } else {
    items[i].Type = MENU_ITEM_NORMAL;
  }
}

}  // namespace win