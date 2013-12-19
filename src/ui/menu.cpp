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

#include "base/std.h"

#include "library/anime_db.h"
#include "library/anime_filter.h"
#include "taiga/settings.h"
#include "base/string.h"
#include "taiga/taiga.h"
#include "theme.h"

#include "dlg/dlg_main.h"
#include "dlg/dlg_season.h"

#define MENU UI.Menus.Menu[menu_index]

// =============================================================================

void UpdateAnimeMenu(anime::Item* anime_item) {
  if (!anime_item) return;
  if (!anime_item->IsInList()) return;
  int item_index = 0, menu_index = 0;

  // Edit > Score
  menu_index = UI.Menus.GetIndex(L"EditScore");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      MENU.Items[i].Checked = false;
      MENU.Items[i].Default = false;
    }
    item_index = anime_item->GetMyScore();
    if (item_index < static_cast<int>(MENU.Items.size())) {
      MENU.Items[item_index].Checked = true;
      MENU.Items[item_index].Default = true;
    }
  }
  // Edit > Status
  menu_index = UI.Menus.GetIndex(L"EditStatus");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      MENU.Items[i].Checked = false;
      MENU.Items[i].Default = false;
    }
    item_index = anime_item->GetMyStatus();
    if (item_index == anime::kPlanToWatch) item_index--;
    if (item_index - 1 < static_cast<int>(MENU.Items.size())) {
      MENU.Items[item_index - 1].Checked = true;
      MENU.Items[item_index - 1].Default = true;
    }
  }

  // Play
  menu_index = UI.Menus.GetIndex(L"RightClick");
  if (menu_index > -1) {
    for (int i = static_cast<int>(MENU.Items.size()) - 1; i > 0; i--) {
      if (MENU.Items[i].Type == win::MENU_ITEM_SEPARATOR) {
        // Clear items
        MENU.Items.resize(i + 1);
        // Play episode
        if (anime_item->GetEpisodeCount() != 1) {
          MENU.CreateItem(L"", L"Play episode", L"PlayEpisode");
        }
        // Play last episode
        if (anime_item->GetMyLastWatchedEpisode() > 0) {
          MENU.CreateItem(
            L"PlayLast()", 
            L"Play last episode (#" + ToWstr(anime_item->GetMyLastWatchedEpisode()) + L")");
        }
        // Play next episode
        if (anime_item->GetEpisodeCount() == 0 ||
            anime_item->GetMyLastWatchedEpisode() < anime_item->GetEpisodeCount()) {
          MENU.CreateItem(
            L"PlayNext()", 
            L"Play next episode (#" + ToWstr(anime_item->GetMyLastWatchedEpisode() + 1) + L")");
        }
        // Play random episode
        if (anime_item->GetEpisodeCount() != 1) {
          MENU.CreateItem(L"PlayRandom()", L"Play random episode");
        }
        break;
      }
    }
  }

  // Play > Episode
  menu_index = UI.Menus.GetIndex(L"PlayEpisode");
  if (menu_index > -1) {
    // Clear menu
    MENU.Items.clear();

    // Add episode numbers
    int count_max, count_column;
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
      if (count_max > 52) count_column *= 2;
      MENU.CreateItem(L"PlayEpisode(" + ToWstr(i) + L")", L"#" + ToWstr(i), L"", 
        i <= anime_item->GetMyLastWatchedEpisode(), 
        false, true, (i > 1) && (i % count_column == 1), false);
    }
  }
}

void UpdateAnnounceMenu() {
  // List > Announce current episode
  int menu_index = UI.Menus.GetIndex(L"List");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      if (MENU.Items[i].SubMenu == L"Announce") {
        MENU.Items[i].Enabled = CurrentEpisode.anime_id > 0;
        break;
      }
    }
  }
}

void UpdateExternalLinksMenu() {
  int menu_index = UI.Menus.GetIndex(L"ExternalLinks");
  if (menu_index > -1) {
    // Clear menu
    MENU.Items.clear();

    vector<wstring> lines;
    Split(Settings[taiga::kApp_Interface_ExternalLinks], L"\r\n", lines);
    for (auto line = lines.begin(); line != lines.end(); ++line) {
      if (IsEqual(*line, L"-")) {
        // Add separator
        MENU.CreateItem();
      } else {
        vector<wstring> content;
        Split(*line, L"|", content);
        if (content.size() > 1) {
          MENU.CreateItem(L"URL(" + content.at(1) + L")", content.at(0));
        }
      }
    }
  }
}

void UpdateFoldersMenu() {
  int menu_index = UI.Menus.GetIndex(L"Folders");
  if (menu_index > -1) {
    // Clear menu
    MENU.Items.clear();

    if (!Settings.root_folders.empty()) {
      // Add folders
      for (unsigned int i = 0; i < Settings.root_folders.size(); i++) {
        MENU.CreateItem(L"Execute(" + Settings.root_folders[i] + L")", Settings.root_folders[i]);
      }
      // Add separator
      MENU.CreateItem();
    }

    // Add default item
    MENU.CreateItem(L"AddFolder()", L"Add new folder...");
  }
}

void UpdateSearchListMenu(bool enabled) {
  int menu_index = UI.Menus.GetIndex(L"SearchList");
  if (menu_index > -1) {
    // Add to list
    for (size_t i = 0; i < MENU.Items.size(); i++) {
      if (MENU.Items[i].SubMenu == L"AddToList") {
        MENU.Items[i].Enabled = enabled;
        break;
      }
    }
  }
}

void UpdateSeasonListMenu(bool enabled) {
  int menu_index = UI.Menus.GetIndex(L"SeasonList");
  if (menu_index > -1) {
    // Add to list
    for (size_t i = 0; i < MENU.Items.size(); i++) {
      if (MENU.Items[i].SubMenu == L"AddToList") {
        MENU.Items[i].Enabled = enabled;
        break;
      }
    }
  }
}

void UpdateSeasonMenu() {
  int item_index, menu_index = -1;
  
  // Group by
  menu_index = UI.Menus.GetIndex(L"SeasonGroup");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      MENU.Items[i].Checked = false;
    }
    item_index = SeasonDialog.group_by;
    if (item_index < static_cast<int>(MENU.Items.size())) {
      MENU.Items[item_index].Checked = true;
    }
  }

  // Sort by
  menu_index = UI.Menus.GetIndex(L"SeasonSort");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      MENU.Items[i].Checked = false;
    }
    item_index = SeasonDialog.sort_by;
    if (item_index < static_cast<int>(MENU.Items.size())) {
      MENU.Items[item_index].Checked = true;
    }
  }

  // View as
  menu_index = UI.Menus.GetIndex(L"SeasonView");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      MENU.Items[i].Checked = false;
    }
    item_index = SeasonDialog.view_as;
    if (item_index < static_cast<int>(MENU.Items.size())) {
      MENU.Items[item_index].Checked = true;
    }
  }
}

void UpdateToolsMenu() {
  int menu_index = UI.Menus.GetIndex(L"Tools");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      // Tools > Enable anime recognition
      if (MENU.Items[i].Action == L"ToggleRecognition()")
        MENU.Items[i].Checked = Settings.GetBool(taiga::kApp_Option_EnableRecognition);
      // Tools > Enable auto sharing
      if (MENU.Items[i].Action == L"ToggleSharing()")
        MENU.Items[i].Checked = Settings.GetBool(taiga::kApp_Option_EnableSharing);
      // Tools > Enable auto synchronization
      if (MENU.Items[i].Action == L"ToggleSynchronization()")
        MENU.Items[i].Checked = Settings.GetBool(taiga::kApp_Option_EnableSync);
    }
  }
}

void UpdateTrayMenu() {
  int menu_index = UI.Menus.GetIndex(L"Tray");
  if (menu_index > -1) {
    // Tray > Enable recognition
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      if (MENU.Items[i].Action == L"ToggleRecognition()") {
        MENU.Items[i].Checked = Settings.GetBool(taiga::kApp_Option_EnableRecognition);
        break;
      }
    }
  }
}

void UpdateViewMenu() {
  int item_index, menu_index = -1;

  menu_index = UI.Menus.GetIndex(L"View");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      MENU.Items[i].Checked = false;
    }
    item_index = MainDialog.navigation.GetCurrentPage();
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      if (MENU.Items[i].Action == L"ViewContent(" + ToWstr(item_index) + L")") {
        MENU.Items[i].Checked = true;
        break;
      }
    }
    MENU.Items.back().Checked = !Settings.GetBool(taiga::kApp_Option_HideSidebar);
  }
}

void UpdateAllMenus(anime::Item* anime_item) {
  UpdateAnimeMenu(anime_item);
  UpdateAnnounceMenu();
  UpdateExternalLinksMenu();
  UpdateFoldersMenu();
  UpdateToolsMenu();
  UpdateTrayMenu();
  UpdateViewMenu();
}