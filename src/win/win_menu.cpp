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

#include "win_menu.h"

namespace win {

HMENU MenuList::CreateNewMenu(const std::wstring& name,
                              std::vector<HMENU>& menu_handles) {
  auto menu = FindMenu(name);

  if (!menu)
    return nullptr;

  HMENU handle = nullptr;
  if (menu->type == L"menubar") {
    handle = ::CreateMenu();
  } else {
    handle = ::CreatePopupMenu();
  }

  if (!handle)
    return nullptr;

  menu_handles.push_back(handle);

  for (auto item = menu->items.begin(); item != menu->items.end(); ++item) {
    if (!item->visible)
      continue;
    const UINT flags =
        (item->checked ? MF_CHECKED : 0) |
        (item->def ? MF_DEFAULT : 0) |
        (item->enabled ? MF_ENABLED : MF_GRAYED) |
        (item->new_column ? MF_MENUBARBREAK : 0) |
        (item->radio ? MFT_RADIOCHECK : 0);
    switch (item->type) {
      case kMenuItemDefault: {
        UINT_PTR id_new_item = reinterpret_cast<UINT_PTR>(&item->action);
        ::AppendMenu(handle, MF_STRING | flags, id_new_item,
                     item->name.c_str());
        if (item->def)
          ::SetMenuDefaultItem(handle, id_new_item, FALSE);
        break;
      }
      case kMenuItemSeparator: {
        ::AppendMenu(handle, MF_SEPARATOR, 0, nullptr);
        break;
      }
      case kMenuItemSubmenu: {
        auto submenu = FindMenu(item->submenu.c_str());
        if (submenu && submenu != menu) {
          HMENU submenu_handle = CreateNewMenu(submenu->name.c_str(),
                                               menu_handles);
          ::AppendMenu(handle, MF_POPUP | flags,
                       reinterpret_cast<UINT_PTR>(submenu_handle),
                       item->name.c_str());
        }
        break;
      }
    }
  }

  return handle;
}

std::wstring MenuList::Show(HWND hwnd, int x, int y, const std::wstring& name) {
  std::vector<HMENU> menu_handles;
  CreateNewMenu(name, menu_handles);

  if (menu_handles.empty())
    return std::wstring();

  if (!x && !y) {
    POINT point;
    ::GetCursorPos(&point);
    x = point.x;
    y = point.y;
  }

  UINT flags = TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD;
  UINT_PTR index = ::TrackPopupMenuEx(menu_handles.front(),
                                      flags, x, y, hwnd, nullptr);

  for (auto it = menu_handles.begin(); it != menu_handles.end(); ++it)
    ::DestroyMenu(*it);

  if (index > 0) {
    auto str = reinterpret_cast<std::wstring*>(index);
    return *str;
  } else {
    return std::wstring();
  }
}

void MenuList::Create(const std::wstring& name, const std::wstring& type) {
  menus[name].name = name;
  menus[name].type = type;
}

Menu* MenuList::FindMenu(const std::wstring& name) {
  auto it = menus.find(name);
  if (it != menus.end())
    return &it->second;

  return nullptr;
}

void Menu::CreateItem(const std::wstring& action, const std::wstring& name,
                      const std::wstring& submenu,
                      bool checked, bool def, bool enabled,
                      bool newcolumn, bool radio) {
  MenuItem item;

  item.action = action;
  item.checked = checked;
  item.def = def;
  item.enabled = enabled;
  item.name = name;
  item.new_column = newcolumn;
  item.radio = radio;
  item.submenu = submenu;
  item.visible = true;

  if (!submenu.empty()) {
    item.type = kMenuItemSubmenu;
  } else if (name.empty()) {
    item.type = kMenuItemSeparator;
  } else {
    item.type = kMenuItemDefault;
  }

  items.push_back(item);
}

}  // namespace win