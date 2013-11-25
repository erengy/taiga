/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

namespace anime {
class Item;
}

void UpdateAllMenus(anime::Item* anime_item = nullptr);
void UpdateAnimeMenu(anime::Item* anime_item);
void UpdateAnnounceMenu();
void UpdateExternalLinksMenu();
void UpdateFoldersMenu();
void UpdateSearchListMenu(bool enabled = false);
void UpdateSeasonListMenu(bool enabled = false);
void UpdateSeasonMenu();
void UpdateToolsMenu();
void UpdateTrayMenu();
void UpdateViewMenu();

#endif  // TAIGA_UI_MENU_H