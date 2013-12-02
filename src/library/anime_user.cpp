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
#include "base/foreach.h"

#include "anime_db.h"
#include "anime_user.h"

#include "history.h"
#include "taiga/http.h"
#include "sync/myanimelist.h"
#include "base/string.h"

anime::Database* anime::ListUser::database_ = &AnimeDatabase;

namespace anime {

// =============================================================================

User::User()
    : id(0) {
}

ListUser::ListUser() {
}

void ListUser::Clear() {
  user_.id = 0;
  user_.name.clear();
}

int ListUser::GetId() const {
  return user_.id;
}

int ListUser::GetItemCount(int status, bool check_events) const {
  int count = 0;

  // Get current count
  foreach_(it, database_->items)
    if (it->second.GetMyStatus(false) == status)
      count++;

  // Search event queue for status changes
  if (check_events) {
    for (auto it = History.queue.items.begin(); it != History.queue.items.end(); ++it) {
      if (it->mode == taiga::kHttpServiceAddLibraryEntry) continue;
      if (it->status) {
        if (status == *it->status) {
          count++;
        } else {
          auto anime_item = database_->FindItem(it->anime_id);
          if (anime_item && status == anime_item->GetMyStatus(false)) {
            count--;
          }
        }
      }
    }
  }

  return count;
}

const wstring& ListUser::GetName() const {
  return user_.name;
}

void ListUser::SetId(int id) {
  user_.id = id;
}

void ListUser::SetName(const wstring& name) {
  user_.name = name;
}

} // namespace anime