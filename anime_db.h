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
  
  // Loads local anime database on program startup from db/anime.xml, returns
  // false if no such database exists.
  bool LoadDatabase();
  // Saves local anime database on program exit to db/anime.xml, returns false
  // if the database is empty.
  bool SaveDatabase();

  // Deletes all user information from anime items.
  void ClearUserData();
  // Loads anime list on startup and list-refresh from
  // user\<username>\anime.xml, returns false if no such list exists.
  bool LoadList();
  // Saves anime list everytime an item is updated to
  // user\<username>\anime.xml, returns false if no such item exists with given
  // ID in the database or the relevant XML node doesn't exist.
  bool SaveList(int anime_id, 
                const wstring& child, const wstring& value, 
                ListSaveMode mode = EDIT_ANIME);
  
  // Searches the database for an item with given ID.
  Item* FindItem(int anime_id);
  // Searches the database for an item with given ID, which has a sequel.
  Item* FindSequel(int anime_id);
  // Updates anime information, or adds a new item if no such anime exists.
  // New information may include both series and user information. Series
  // information is updated depending on its last_modified value.
  void UpdateItem(Item& item);

  // Deletes user information from an item, after HTTP_MAL_AnimeDelete
  // succeeds.
  bool DeleteListItem(int anime_id);
  // Current item is the item selected on MainDialog's ListView.
  int GetCurrentId();
  Item* GetCurrentItem();
  void SetCurrentId(int anime_id);

  // Anime items are mapped to their IDs.
  std::map<int, Item> items;

  // Read from user\<username>\anime.xml.
  ListUser user;

 private:
  // Thread safety
  win32::CriticalSection critical_section_;
  
  int current_id_;
  
  wstring file_;
  wstring folder_;
};

class SeasonDatabase {
 public:
  SeasonDatabase();
  virtual ~SeasonDatabase() {}

  // Loads season data from db\season\<seasonname>.xml, returns false if no such
  // file exists.
  bool Load(wstring file);

  // Only IDs are stored here, actual info is kept in Database.
  vector<int> items;

  // TODO: Remove
  time_t last_modified;
  
  // Season name (e.g. Spring 2012)
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