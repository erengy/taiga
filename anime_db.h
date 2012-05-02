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

#ifndef ANIMEDB_H
#define ANIMEDB_H

#include "std.h"

#include "anime_item.h"
#include "anime_user.h"

#include "win32/win_thread.h"

namespace anime {

// =============================================================================

// Used in Database::SaveList()
enum ListSaveMode {
  ADD_ANIME,
  DELETE_ANIME,
  EDIT_ANIME,
  EDIT_USER
};

class Database {
 public:
  Database();
  virtual ~Database() {}

  // called only once on program startup
  // returns false if no local database exists.
  bool LoadDatabase();
  // called everytime the database is updated?
  // is there a performance issue?
  bool SaveDatabase();

  void ClearUserData();
  // done on startup and when list is refreshed
  bool LoadList();
  // done everytime an item is updated
  bool SaveList(int anime_id, const wstring& child, const wstring& value, 
                ListSaveMode mode = EDIT_ANIME);

  // called from Item::Edit (after HTTP_MAL_AnimeDelete succeeds)
  bool DeleteItem(int anime_id);
  // called from everywhere!
  Item* FindItem(int anime_id);
  //
  Item* FindSequel(int anime_id);
  // called after parsing search results
  // called from Load functions
  // called from AddToListAs action (from search and season dialogs)
  void UpdateItem(Item& item);

  // current item is the item selected on MainDialog's ListView
  int GetCurrentId();
  Item* GetCurrentItem();
  void SetCurrentId(int anime_id);

  // should we use another container?
  vector<Item> items;

  // read from <username>.xml
  ListUser user;

 private:
  // thread safety
  win32::CriticalSection critical_section_;
  int current_id_;
  wstring file_;
  wstring folder_;
};

class SeasonDatabase {
 public:
  SeasonDatabase();
  virtual ~SeasonDatabase() {}

  bool Load(wstring file);

  // only IDs are stored, actual info is stored in Database
  vector<int> items;
  // not internally modified?
  time_t last_modified;
  // season name (e.g. Spring 2012)
  wstring name;

 private:
  wstring file_;
  wstring folder_;
};

} // namespace anime

// Global objects
extern anime::Database AnimeDatabase;
extern anime::SeasonDatabase SeasonDatabase;

#endif // ANIMEDB_H