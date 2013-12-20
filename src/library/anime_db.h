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

#ifndef TAIGA_LIBRARY_ANIME_DB_H
#define TAIGA_LIBRARY_ANIME_DB_H

#include "anime_item.h"

#include "base/gfx.h"

#include "win/win_thread.h"

class HistoryItem;

namespace anime {

class Database {
public:
  bool LoadDatabase();
  bool SaveDatabase();

  Item* FindItem(int anime_id);
  Item* FindSequel(int anime_id);

  void ClearInvalidItems();
  void UpdateItem(const Item& item);

public:
  bool LoadList();
  bool SaveList();

  int GetItemCount(int status, bool check_history = true);

  void ClearUserData();
  bool DeleteListItem(int anime_id);
  void UpdateItem(const HistoryItem& history_item);

public:
  std::map<int, Item> items;

private:
  win::CriticalSection critical_section_;
};

class ImageDatabase {
public:
  ImageDatabase() {}
  virtual ~ImageDatabase() {}

  // Loads a picture into memory, downloads a new file if requested.
  bool Load(int anime_id, bool load, bool download);

  // Releases image data from memory if an image is not in sight.
  void FreeMemory();

  // Returns a pointer to requested image if available.
  base::Image* GetImage(int anime_id);

private:
  std::map<int, base::Image> items_;
};

}  // namespace anime

// Global objects
extern anime::Database AnimeDatabase;
extern anime::ImageDatabase ImageDatabase;

#endif  // TAIGA_LIBRARY_ANIME_DB_H