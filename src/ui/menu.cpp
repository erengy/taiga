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

#include "base/foreach.h"
#include "base/string.h"
#include "base/xml.h"
#include "library/anime_db.h"
#include "library/anime_episode.h"
#include "taiga/settings.h"
#include "ui/menu.h"

#include "dlg/dlg_main.h"
#include "dlg/dlg_season.h"

namespace ui {

MenuList Menus;

void MenuList::Load() {
  menu_list_.menus.clear();

  wstring menu_resource;
  ReadStringFromResource(L"IDR_MENU", L"DATA", menu_resource);

  xml_document document;
  xml_parse_result parse_result = document.load(menu_resource.data());

  xml_node menus = document.child(L"menus");

  foreach_xmlnode_(menu, menus, L"menu") {
    menu_list_.Create(menu.attribute(L"name").value(),
                      menu.attribute(L"type").value());
    foreach_xmlnode_(item, menu, L"item") {
      menu_list_.menus.back().CreateItem(
          item.attribute(L"action").value(),
          item.attribute(L"name").value(),
          item.attribute(L"sub").value(),
          item.attribute(L"checked").as_bool(),
          item.attribute(L"default").as_bool(),
          !item.attribute(L"disabled").as_bool(),
          item.attribute(L"column").as_bool(),
          item.attribute(L"radio").as_bool());
    }
  }
}

std::wstring MenuList::Show(HWND hwnd, int x, int y, LPCWSTR lpName) {
  return menu_list_.Show(hwnd, x, y, lpName);
}

void MenuList::UpdateAnime(const anime::Item* anime_item) {
  if (!anime_item)
    return;
  if (!anime_item->IsInList())
    return;

  // Edit > Score
  auto menu = menu_list_.FindMenu(L"EditScore");
  if (menu) {
    foreach_(it, menu->items) {
      it->checked = false;
      it->def = false;
    }
    int item_index = anime_item->GetMyScore();
    if (item_index < static_cast<int>(menu->items.size())) {
      menu->items[item_index].checked = true;
      menu->items[item_index].def = true;
    }
  }
  // Edit > Status
  menu = menu_list_.FindMenu(L"EditStatus");
  if (menu) {
    foreach_(it, menu->items) {
      it->checked = false;
      it->def = false;
    }
    int item_index = anime_item->GetMyStatus();
    if (item_index == anime::kPlanToWatch)
      item_index--;
    if (item_index - 1 < static_cast<int>(menu->items.size())) {
      menu->items[item_index - 1].checked = true;
      menu->items[item_index - 1].def = true;
    }
  }

  // Play
  menu = menu_list_.FindMenu(L"RightClick");
  if (menu) {
    for (int i = static_cast<int>(menu->items.size()) - 1; i > 0; i--) {
      if (menu->items[i].type == win::kMenuItemSeparator) {
        // Clear items
        menu->items.resize(i + 1);
        // Play episode
        if (anime_item->GetEpisodeCount() != 1) {
          menu->CreateItem(L"", L"Play episode", L"PlayEpisode");
        }
        // Play last episode
        if (anime_item->GetMyLastWatchedEpisode() > 0) {
          menu->CreateItem(L"PlayLast()",
                           L"Play last episode (#" +
                           ToWstr(anime_item->GetMyLastWatchedEpisode()) + L")");
        }
        // Play next episode
        if (anime_item->GetEpisodeCount() == 0 ||
            anime_item->GetMyLastWatchedEpisode() < anime_item->GetEpisodeCount()) {
          menu->CreateItem(L"PlayNext()",
                           L"Play next episode (#" +
                           ToWstr(anime_item->GetMyLastWatchedEpisode() + 1) + L")");
        }
        // Play random episode
        if (anime_item->GetEpisodeCount() != 1) {
          menu->CreateItem(L"PlayRandom()", L"Play random episode");
        }
        break;
      }
    }
  }

  // Play > Episode
  menu = menu_list_.FindMenu(L"PlayEpisode");
  if (menu) {
    // Clear menu
    menu->items.clear();

    // Add episode numbers
    int count_max = 0;
    int count_column = 0;
    if (anime_item->GetEpisodeCount() > 0) {
      count_max = anime_item->GetEpisodeCount();
    } else {
      count_max = anime_item->GetEpisodeCount();
      if (count_max == 0) {
        count_max = anime_item->GetMyLastWatchedEpisode() + 1;
      }
    }
    for (int i = 1; i <= count_max; i++) {
      count_column = count_max % 12 == 0 ? 12 : 13;
      if (count_max > 52)
        count_column *= 2;
      menu->CreateItem(L"PlayEpisode(" + ToWstr(i) + L")",
                       L"#" + ToWstr(i),
                       L"",
                       i <= anime_item->GetMyLastWatchedEpisode(),
                       false,
                       true,
                       (i > 1) && (i % count_column == 1),
                       false);
    }
  }
}

void MenuList::UpdateAnnounce() {
  // List > Announce current episode
  auto menu = menu_list_.FindMenu(L"List");
  if (menu) {
    foreach_(it, menu->items) {
      if (it->submenu == L"Announce") {
        it->enabled = CurrentEpisode.anime_id > 0;
        break;
      }
    }
  }
}

void MenuList::UpdateExternalLinks() {
  auto menu = menu_list_.FindMenu(L"ExternalLinks");
  if (menu) {
    // Clear menu
    menu->items.clear();

    vector<wstring> lines;
    Split(Settings[taiga::kApp_Interface_ExternalLinks], L"\r\n", lines);
    foreach_(line, lines) {
      if (IsEqual(*line, L"-")) {
        // Add separator
        menu->CreateItem();
      } else {
        vector<wstring> content;
        Split(*line, L"|", content);
        if (content.size() > 1) {
          menu->CreateItem(L"URL(" + content.at(1) + L")", content.at(0));
        }
      }
    }
  }
}

void MenuList::UpdateFolders() {
  auto menu = menu_list_.FindMenu(L"Folders");
  if (menu) {
    // Clear menu
    menu->items.clear();

    if (!Settings.root_folders.empty()) {
      // Add folders
      foreach_(it, Settings.root_folders) {
        menu->CreateItem(L"Execute(" + *it + L")", *it);
      }
      // Add separator
      menu->CreateItem();
    }

    // Add default item
    menu->CreateItem(L"AddFolder()", L"Add new folder...");
  }
}

void MenuList::UpdateSearchList(bool enabled) {
  auto menu = menu_list_.FindMenu(L"SearchList");
  if (menu) {
    // Add to list
    foreach_(it, menu->items) {
      if (it->submenu == L"AddToList") {
        it->enabled = enabled;
        break;
      }
    }
  }
}

void MenuList::UpdateSeasonList(bool enabled) {
  auto menu = menu_list_.FindMenu(L"SeasonList");
  if (menu) {
    // Add to list
    foreach_(it, menu->items) {
      if (it->submenu == L"AddToList") {
        it->enabled = enabled;
        break;
      }
    }
  }
}

void MenuList::UpdateSeason() {
  // Group by
  auto menu = menu_list_.FindMenu(L"SeasonGroup");
  if (menu) {
    foreach_(it, menu->items) {
      it->checked = false;
    }
    int item_index = SeasonDialog.group_by;
    if (item_index < static_cast<int>(menu->items.size())) {
      menu->items[item_index].checked = true;
    }
  }

  // Sort by
  menu = menu_list_.FindMenu(L"SeasonSort");
  if (menu) {
    foreach_(it, menu->items) {
      it->checked = false;
    }
    int item_index = SeasonDialog.sort_by;
    if (item_index < static_cast<int>(menu->items.size())) {
      menu->items[item_index].checked = true;
    }
  }

  // View as
  menu = menu_list_.FindMenu(L"SeasonView");
  if (menu) {
    foreach_(it, menu->items) {
      it->checked = false;
    }
    int item_index = SeasonDialog.view_as;
    if (item_index < static_cast<int>(menu->items.size())) {
      menu->items[item_index].checked = true;
    }
  }
}

void MenuList::UpdateTools() {
  auto menu = menu_list_.FindMenu(L"Tools");
  if (menu) {
    foreach_(it, menu->items) {
      // Tools > Enable anime recognition
      if (it->action == L"ToggleRecognition()")
        it->checked = Settings.GetBool(taiga::kApp_Option_EnableRecognition);
      // Tools > Enable auto sharing
      if (it->action == L"ToggleSharing()")
        it->checked = Settings.GetBool(taiga::kApp_Option_EnableSharing);
      // Tools > Enable auto synchronization
      if (it->action == L"ToggleSynchronization()")
        it->checked = Settings.GetBool(taiga::kApp_Option_EnableSync);
    }
  }
}

void MenuList::UpdateTray() {
  auto menu = menu_list_.FindMenu(L"Tray");
  if (menu) {
    // Tray > Enable recognition
    foreach_(it, menu->items) {
      if (it->action == L"ToggleRecognition()") {
        it->checked = Settings.GetBool(taiga::kApp_Option_EnableRecognition);
        break;
      }
    }
  }
}

void MenuList::UpdateView() {
  auto menu = menu_list_.FindMenu(L"View");
  if (menu) {
    foreach_(it, menu->items) {
      it->checked = false;
    }
    int item_index = DlgMain.navigation.GetCurrentPage();
    foreach_(it, menu->items) {
      if (it->action == L"ViewContent(" + ToWstr(item_index) + L")") {
        it->checked = true;
        break;
      }
    }
    menu->items.back().checked = !Settings.GetBool(taiga::kApp_Option_HideSidebar);
  }
}

void MenuList::UpdateAll(const anime::Item* anime_item) {
  UpdateAnime(anime_item);
  UpdateAnnounce();
  UpdateExternalLinks();
  UpdateFolders();
  UpdateTools();
  UpdateTray();
  UpdateView();
}

}  // namespace ui