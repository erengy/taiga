/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>
#include <map>

#include "media/anime.h"
#include "media/anime_item.h"

namespace library {
struct QueueItem;
}
namespace pugi {
class xml_document;
class xml_node;
}
namespace sync {
enum class ServiceId;
}

namespace anime {

class Database {
public:
  bool LoadDatabase();
  bool SaveDatabase() const;

  Item* Find(int id, bool log_error = true);
  Item* Find(const std::wstring& id, sync::ServiceId service,
             bool log_error = true);

  void ClearInvalidItems();
  bool DeleteItem(int id);

public:
  bool LoadList();
  bool SaveList(bool include_database = false) const;

  int GetItemCount(MyStatus status, bool check_history = true);

  void AddToList(int anime_id, MyStatus status);
  void ClearUserData();
  bool DeleteListItem(int anime_id);
  void UpdateItem(const library::QueueItem& queue_item);

public:
  std::map<int, Item> items;

private:
  void ReadDatabaseNode(pugi::xml_node& database_node);
  void WriteDatabaseNode(pugi::xml_node& database_node) const;

  void HandleCompatibility(const std::wstring& meta_version);
  void HandleListCompatibility(const std::wstring& meta_version);
};

inline Database db;

}  // namespace anime
