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

namespace win32 {

// =============================================================================

HMENU MenuList::CreateNewMenu(LPCWSTR lpName, vector<HMENU>& hMenu) {
  // Initialize
  int menu_index = GetIndex(lpName);
  if (menu_index == -1) return NULL;
  int nMenu = hMenu.size();
  hMenu.resize(nMenu + 1);

  // Create menu
  if (Menu[menu_index].Type == L"menubar") {
    hMenu[nMenu] = ::CreateMenu();
  } else {
    hMenu[nMenu] = ::CreatePopupMenu();
  }
  if (!hMenu[nMenu]) return NULL;

  // Add items
  for (unsigned int i = 0; i < Menu[menu_index].Items.size(); i++) {
    UINT uFlags = (Menu[menu_index].Items[i].Checked ? MF_CHECKED : NULL) | 
      (Menu[menu_index].Items[i].Default ? MF_DEFAULT : NULL) | 
      (Menu[menu_index].Items[i].Enabled ? MF_ENABLED : MF_GRAYED) | 
      (Menu[menu_index].Items[i].NewColumn ? MF_MENUBARBREAK : NULL) | 
      (Menu[menu_index].Items[i].Radio ? MFT_RADIOCHECK : NULL);
    
    switch (Menu[menu_index].Items[i].Type) {
      // Normal item      
      case MENU_ITEM_NORMAL: {
        UINT_PTR uIDNewItem = (UINT_PTR)&Menu[menu_index].Items[i].Action;
        ::AppendMenu(hMenu[nMenu], MF_STRING | uFlags, uIDNewItem, 
          Menu[menu_index].Items[i].Name.c_str());
        if (Menu[menu_index].Items[i].Default) {
          ::SetMenuDefaultItem(hMenu[nMenu], uIDNewItem, FALSE);
        }
        // TEMP
        /*Menu[menu_index].Items[i].Icon.Load(ImageList_GetIcon(m_hImageList, 1, ILD_NORMAL));
        BOOL bReturn = ::SetMenuItemBitmaps(hMenu[nMenu], uIDNewItem, MF_BITMAP | MF_BYCOMMAND, 
          Menu[menu_index].Items[i].Icon.Handle, NULL);*/
        break;
      }
      // Separator
      case MENU_ITEM_SEPARATOR: {
        ::AppendMenu(hMenu[nMenu], MF_SEPARATOR, NULL, NULL);
        break;
      }
      // Sub menu
      case MENU_ITEM_SUBMENU: {
        int sub_index = GetIndex(Menu[menu_index].Items[i].SubMenu.c_str());
        if (sub_index > -1 && sub_index != menu_index) {
          HMENU hSubMenu = CreateNewMenu(Menu[sub_index].Name.c_str(), hMenu);
          ::AppendMenu(hMenu[nMenu], MF_POPUP | uFlags, 
            (UINT_PTR)hSubMenu, Menu[menu_index].Items[i].Name.c_str());
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
  if (!hMenu.size()) return L"";

  // Set position
  if (x == 0 && y == 0) {
    POINT point;
    ::GetCursorPos(&point);
    x = point.x; y = point.y;
  }
  
  // Show menu
  UINT_PTR index = ::TrackPopupMenuEx(hMenu[0], 
    TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD, 
    x, y, hwnd, NULL);

  // Clean-up
  for (unsigned int i = 0; i < hMenu.size(); i++) {
    ::DestroyMenu(hMenu[i]);
  }
  /*for (int i = 1; i < Count; i++) {
    for (int j = 1; j < Menu[i].Count; i++) {
      Menu[i].Items[j].Icon.Destroy();
    }
  }*/

  // Return action
  if (index > 0) {
    wstring* str = reinterpret_cast<wstring*>(index);
    return *str;
  } else {
    return L"";
  }
}

// =============================================================================

void MenuList::Create(LPCWSTR lpName, LPCWSTR lpType) {
  Menu.resize(Menu.size() + 1);
  Menu.back().Name = lpName;
  Menu.back().Type = lpType;
}

int MenuList::GetIndex(LPCWSTR lpName) {
  for (unsigned int i = 0; i < Menu.size(); i++) {
    if (Menu[i].Name == lpName) return i;
  }
  return -1;
}

void MenuList::SetImageList(HIMAGELIST hImageList) {
  m_hImageList = hImageList;
}

// =============================================================================

void MenuList::Menu::CreateItem(wstring action, wstring name, wstring sub,
                                bool checked, bool def, bool enabled, 
                                bool newcolumn, bool radio) {
  unsigned int i = Items.size();
  Items.resize(i + 1);
  
  Items[i].Action    = action;
  Items[i].Checked   = checked;
  Items[i].Default   = def;
  Items[i].Enabled   = enabled;
  Items[i].Name      = name;
  Items[i].NewColumn = newcolumn;
  Items[i].Radio     = radio;
  Items[i].SubMenu   = sub;

  if (!sub.empty()) {
    Items[i].Type = MENU_ITEM_SUBMENU;
  } else if (name.empty()) {
    Items[i].Type = MENU_ITEM_SEPARATOR;
  } else {
    Items[i].Type = MENU_ITEM_NORMAL;
  }

  Items[i].Icon.Index = 0;
  Items[i].Icon.Destroy();
}

// =============================================================================

void MenuList::Menu::MenuItem::CMenuIcon::Destroy() {
  ::DeleteObject(Handle);
  Handle = NULL;
}

void MenuList::Menu::MenuItem::CMenuIcon::Load(HICON hIcon) {
  Destroy();
  
  RECT rect = {0};
  rect.right = ::GetSystemMetrics(SM_CXMENUCHECK);
  rect.bottom = ::GetSystemMetrics(SM_CYMENUCHECK);

  HWND hDesktop = ::GetDesktopWindow();
  HDC hScreen = ::GetDC(hDesktop);
  HDC hDC = ::CreateCompatibleDC(hScreen);
  Handle = ::CreateCompatibleBitmap(hScreen, rect.right, rect.bottom);
  HBITMAP hOld = (HBITMAP)::SelectObject(hDC, Handle);
  
  ::SetBkColor(hDC, ::GetSysColor(COLOR_MENU));
  ::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
  ::DrawIconEx(hDC, 0, 0, hIcon, rect.right, rect.bottom, 0, NULL, DI_NORMAL);

  ::DeleteObject(hOld);
  ::DeleteDC(hDC);
  ::ReleaseDC(hDesktop, hScreen);
  ::DestroyIcon(hIcon);
}

} // namespace win32