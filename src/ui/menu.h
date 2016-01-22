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

#ifndef TAIGA_UI_MENU_H
#define TAIGA_UI_MENU_H

#include <string>

#include "win/win_menu.h"

namespace anime {
class Item;
}
class FeedItem;

namespace ui {

class MenuList {
public:
  void Load();

  std::wstring Show(HWND hwnd, int x, int y, LPCWSTR lpName);

  void UpdateAll(const anime::Item* anime_item = nullptr);
  void UpdateAnime(const anime::Item* anime_item);
  void UpdateAnimeListHeaders();
  void UpdateAnnounce();
  void UpdateExternalLinks();
  void UpdateFolders();
  void UpdateHistoryList(bool enabled = false);
  void UpdateScore(const anime::Item* anime_item);
  void UpdateSearchList(bool enabled = false);
  void UpdateSeasonList(bool enabled = false);
  void UpdateTorrentsList(const FeedItem& feed_item);
  void UpdateSeason();
  void UpdateTools();
  void UpdateTray();
  void UpdateView();

private:
  win::MenuList menu_list_;
};

extern MenuList Menus;

}  // namespace ui

#endif  // TAIGA_UI_MENU_H