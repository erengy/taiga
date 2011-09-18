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

#include "std.h"
#include "animelist.h"
#include "dlg/dlg_main.h"
#include "dlg/dlg_season.h"
#include "myanimelist.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "theme.h"

#define MENU UI.Menus.Menu[menu_index]

// =============================================================================

void UpdateAccountMenu() {
  int menu_index = UI.Menus.GetIndex(L"Account");
  if (menu_index > -1) {
    // Account > Log in
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      if (MENU.Items[i].Action == L"LoginLogout()") {
        MENU.Items[i].Name = Taiga.logged_in ? L"Log out" : L"Log in";
        break;
      }
    }
    // Account > Enable recognition
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      if (MENU.Items[i].Action == L"ToggleRecognition()") {
        MENU.Items[i].Checked = Taiga.is_recognition_enabled;
        break;
      }
    }
  }
}

void UpdateAnimeMenu(Anime* anime) {
  if (!anime) return;
  int item_index = 0, menu_index = 0;

  // Edit > Score
  menu_index = UI.Menus.GetIndex(L"EditScore");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      MENU.Items[i].Checked = false;
      MENU.Items[i].Default = false;
    }
    item_index = anime->GetScore();
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
    item_index = anime->GetStatus();
    if (item_index == MAL_PLANTOWATCH) item_index--;
    if (item_index - 1 < static_cast<int>(MENU.Items.size())) {
      MENU.Items[item_index - 1].Checked = true;
      MENU.Items[item_index - 1].Default = true;
    }
  }

  // Play
  menu_index = UI.Menus.GetIndex(L"Play");
  if (menu_index > -1) {
    // Clear menu
    MENU.Items.clear();

    // Add items
    MENU.CreateItem(L"PlayLast()", 
      L"Last watched (#" + ToWSTR(anime->GetLastWatchedEpisode()) + L")", 
      L"", false, false, 
      anime->GetLastWatchedEpisode() > 0);
    MENU.CreateItem(L"PlayNext()", 
      L"Next episode (#" + ToWSTR(anime->GetLastWatchedEpisode() + 1) + L")", 
      L"", false, false, 
      anime->GetLastWatchedEpisode() < anime->series_episodes || anime->series_episodes == 0);
    MENU.CreateItem(L"PlayRandom()", L"Random episode");
    MENU.CreateItem();
    MENU.CreateItem(L"", L"Episode", L"PlayEpisode");
  }

  // Play > Episode
  menu_index = UI.Menus.GetIndex(L"PlayEpisode");
  if (menu_index > -1) {
    // Clear menu
    MENU.Items.clear();

    // Add episode numbers
    int count_max, count_column;
    if (anime->series_episodes > 0) {
      count_max = anime->series_episodes;
    } else {
      count_max = anime->GetTotalEpisodes();
      if (count_max == 0) {
        count_max = anime->GetLastWatchedEpisode() + 1;
      }
    }
    for (int i = 1; i <= count_max; i++) {
      count_column = count_max % 12 == 0 ? 12 : 13;
      if (count_max > 52) count_column *= 2;
      MENU.CreateItem(L"PlayEpisode(" + ToWSTR(i) + L")", L"#" + ToWSTR(i), L"", 
        i <= anime->GetLastWatchedEpisode(), 
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

void UpdateFilterMenu() {
  // Filter > Status
  int menu_index = UI.Menus.GetIndex(L"FilterStatus");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      MENU.Items[i].Checked = AnimeList.filters.status[i] == TRUE;
    }
  }
  // Filter > Type
  menu_index = UI.Menus.GetIndex(L"FilterType");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      MENU.Items[i].Checked = AnimeList.filters.type[i] == TRUE;
    }
  }
}

void UpdateFoldersMenu() {
  int menu_index = UI.Menus.GetIndex(L"Folders");
  if (menu_index > -1) {
    // Clear menu
    MENU.Items.clear();

    if (!Settings.Folders.root.empty()) {
      // Add folders
      for (unsigned int i = 0; i < Settings.Folders.root.size(); i++) {
        MENU.CreateItem(L"Execute(" + Settings.Folders.root[i] + L")", Settings.Folders.root[i]);
      }
      // Add separator
      MENU.CreateItem();
    }

    // Add default item
    MENU.CreateItem(L"AddFolder()", L"Add new folder...");
  }
}

void UpdateSearchMenu() {
  int menu_index = UI.Menus.GetIndex(L"SearchBar");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      MENU.Items[i].Checked = false;
    }
    MENU.Items[0].Checked = MainDialog.search_bar.filter_list;
    if (MainDialog.search_bar.index < MENU.Items.size()) {
      MENU.Items[MainDialog.search_bar.index].Checked = true;
    }
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
}

void UpdateTrayMenu() {
  int menu_index = UI.Menus.GetIndex(L"Tray");
  if (menu_index > -1) {
    // Tray > Enable recognition
    for (unsigned int i = 0; i < MENU.Items.size(); i++) {
      if (MENU.Items[i].Action == L"ToggleRecognition()") {
        MENU.Items[i].Checked = Taiga.is_recognition_enabled;
        break;
      }
    }
  }
}

void UpdateAllMenus(Anime* anime) {
  UpdateAccountMenu();
  UpdateAnimeMenu(anime);
  UpdateAnnounceMenu();
  UpdateFilterMenu();
  UpdateFoldersMenu();
  UpdateSearchMenu();
  UpdateTrayMenu();
}