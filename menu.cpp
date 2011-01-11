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
    for (unsigned int i = 0; i < MENU.Item.size(); i++) {
      if (MENU.Item[i].Action == L"LoginLogout()") {
        MENU.Item[i].Name = Taiga.LoggedIn ? L"Log out" : L"Log in";
        break;
      }
    }
    // Account > Toggle updates
    for (unsigned int i = 0; i < MENU.Item.size(); i++) {
      if (MENU.Item[i].Action == L"ToggleUpdates()") {
        MENU.Item[i].Checked = Taiga.UpdatesEnabled;
        break;
      }
    }
  }
}

void UpdateAnimeMenu(int anime_index) {
  if (anime_index < 1) return;
  int item_index = 0, menu_index = 0;

  // Edit > Score
  menu_index = UI.Menus.GetIndex(L"EditScore");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Item.size(); i++) {
      MENU.Item[i].Checked = false;
      MENU.Item[i].Default = false;
    }
    item_index = AnimeList.Item[anime_index].GetScore();
    if (item_index < static_cast<int>(MENU.Item.size())) {
      MENU.Item[item_index].Checked = true;
      MENU.Item[item_index].Default = true;
    }
  }
  // Edit > Status
  menu_index = UI.Menus.GetIndex(L"EditStatus");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Item.size(); i++) {
      MENU.Item[i].Checked = false;
      MENU.Item[i].Default = false;
    }
    item_index = AnimeList.Item[anime_index].GetStatus();
    if (item_index == MAL_PLANTOWATCH) item_index--;
    if (item_index - 1 < static_cast<int>(MENU.Item.size())) {
      MENU.Item[item_index - 1].Checked = true;
      MENU.Item[item_index - 1].Default = true;
    }
  }

  // Play
  menu_index = UI.Menus.GetIndex(L"Play");
  if (menu_index > -1) {
    // Clear menu
    MENU.Item.clear();

    // Add items
    MENU.CreateItem(L"PlayLast()", 
      L"Last watched (#" + ToWSTR(AnimeList.Item[anime_index].GetLastWatchedEpisode()) + L")", 
      L"", false, false, AnimeList.Item[anime_index].GetLastWatchedEpisode() > 0);
    MENU.CreateItem(L"PlayNext()", 
      L"Next episode (#" + ToWSTR(AnimeList.Item[anime_index].GetLastWatchedEpisode() + 1) + L")", 
      L"", false, false, AnimeList.Item[anime_index].GetStatus() != MAL_COMPLETED);
    MENU.CreateItem(L"PlayRandom()", L"Random episode");
    MENU.CreateItem();
    MENU.CreateItem(L"", L"Episode", L"PlayEpisode");
  }

  // Play > Episode
  menu_index = UI.Menus.GetIndex(L"PlayEpisode");
  if (menu_index > -1) {
    // Clear menu
    MENU.Item.clear();

    // Add episode numbers
    int count_max, count_column;
    if (AnimeList.Item[anime_index].Series_Episodes > 0) {
      count_max = AnimeList.Item[anime_index].Series_Episodes;
    } else {
      count_max = AnimeList.Item[anime_index].GetTotalEpisodes();
      if (count_max == 0) {
        count_max = AnimeList.Item[anime_index].GetLastWatchedEpisode() + 1;
      }
    }
    for (int i = 1; i <= count_max; i++) {
      count_column = count_max % 12 == 0 ? 12 : 13;
      if (count_max > 52) count_column *= 2;
      MENU.CreateItem(L"PlayEpisode(" + ToWSTR(i) + L")", L"#" + ToWSTR(i), L"", 
        i <= AnimeList.Item[anime_index].GetLastWatchedEpisode(), 
        false, true, (i > 1) && (i % count_column == 1), false);
    }
  }
}

void UpdateAnnounceMenu(int anime_index) {
  // List > Announce current episode
  int menu_index = UI.Menus.GetIndex(L"List");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Item.size(); i++) {
      if (MENU.Item[i].SubMenu == L"Announce") {
        MENU.Item[i].Enabled = CurrentEpisode.Index > 0;
        break;
      }
    }
  }
}

void UpdateFilterMenu() {
  // Filter > Status
  int menu_index = UI.Menus.GetIndex(L"FilterStatus");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Item.size(); i++) {
      MENU.Item[i].Checked = AnimeList.Filter.Status[i] == TRUE;
    }
  }
  // Filter > Type
  menu_index = UI.Menus.GetIndex(L"FilterType");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Item.size(); i++) {
      MENU.Item[i].Checked = AnimeList.Filter.Type[i] == TRUE;
    }
  }
}

void UpdateFoldersMenu() {
  int menu_index = UI.Menus.GetIndex(L"Folders");
  if (menu_index > -1) {
    // Clear menu
    MENU.Item.clear();

    if (!Settings.Folders.Root.empty()) {
      // Add folders
      for (unsigned int i = 0; i < Settings.Folders.Root.size(); i++) {
        MENU.CreateItem(L"Execute(" + Settings.Folders.Root[i] + L")", Settings.Folders.Root[i]);
      }
      // Add seperator
      MENU.CreateItem();
    }

    // Add default item
    MENU.CreateItem(L"AddFolder()", L"Add new folder...");
  }
}

void UpdateSearchMenu() {
  int menu_index = UI.Menus.GetIndex(L"SearchBar");
  if (menu_index > -1) {
    for (unsigned int i = 0; i < MENU.Item.size(); i++) {
      MENU.Item[i].Checked = FALSE;
    }
    if (MainWindow.m_SearchBar.Index < MENU.Item.size()) {
      MENU.Item[MainWindow.m_SearchBar.Index].Checked = TRUE;
    }
  }
}

void UpdateSearchListMenu(bool enabled) {
  int menu_index = UI.Menus.GetIndex(L"SearchList");
  if (menu_index > -1) {
    // Add to list
    for (size_t i = 0; i < MENU.Item.size(); i++) {
      if (MENU.Item[i].SubMenu == L"AddToList") {
        MENU.Item[i].Enabled = enabled;
        break;
      }
    }
  }
}

void UpdateTrayMenu() {
  int menu_index = UI.Menus.GetIndex(L"Tray");
  if (menu_index > -1) {
    // Tray > Toggle updates
    for (unsigned int i = 0; i < MENU.Item.size(); i++) {
      if (MENU.Item[i].Action == L"ToggleUpdates()") {
        MENU.Item[i].Checked = Taiga.UpdatesEnabled;
        break;
      }
    }
  }
}

void UpdateAllMenus(int anime_index) {
  UpdateAccountMenu();
  UpdateAnimeMenu(anime_index);
  UpdateAnnounceMenu(anime_index);
  UpdateFilterMenu();
  UpdateFoldersMenu();
  UpdateSearchMenu();
  UpdateTrayMenu();
}