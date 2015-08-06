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

#ifndef TAIGA_LIBRARY_ANIME_DB_H
#define TAIGA_LIBRARY_ANIME_DB_H

#include <map>

#include "library/anime_item.h"

class HistoryItem;
namespace pugi {
class xml_document;
class xml_node;
}

namespace anime {

class Database {
public:
  bool LoadDatabase();
  bool SaveDatabase();

  Item* FindItem(int id, bool log_error = true);
  Item* FindItem(const std::wstring& id, enum_t service, bool log_error = true);

  void ClearInvalidItems();
  bool DeleteItem(int id);
  int UpdateItem(const Item& item);

public:
  bool LoadList();
  bool SaveList(bool include_database = false);

  int GetItemCount(int status, bool check_history = true);

  void AddToList(int anime_id, int status);
  void ClearUserData();
  bool DeleteListItem(int anime_id);
  void UpdateItem(const HistoryItem& history_item);

public:
  std::map<int, Item> items;

private:
  void ReadDatabaseNode(pugi::xml_node& database_node);
  void WriteDatabaseNode(pugi::xml_node& database_node);

  bool CheckOldUserDirectory();
  void HandleCompatibility(const std::wstring& meta_version);
  void ReadDatabaseInCompatibilityMode(pugi::xml_document& document);
  void ReadListInCompatibilityMode(pugi::xml_document& document);
};

}  // namespace anime

extern anime::Database AnimeDatabase;

#endif  // TAIGA_LIBRARY_ANIME_DB_H