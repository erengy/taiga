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
    : id(0), 
      watching(0), 
      completed(0), 
      on_hold(0), 
      dropped(0), 
      plan_to_watch(0) {
}

ListUser::ListUser() {
}

void ListUser::Clear() {
  user_.id = 0;
  user_.watching = 0;
  user_.completed = 0;
  user_.on_hold = 0;
  user_.dropped = 0;
  user_.plan_to_watch = 0;
  user_.name.clear();
  user_.days_spent_watching.clear();
}

int ListUser::GetId() const {
  return user_.id;
}

int ListUser::GetItemCount(int status, bool check_events) const {
  int count = 0;
  
  // Get current count
  switch (status) {
    case sync::myanimelist::kWatching:
      count = user_.watching;
      break;
    case sync::myanimelist::kCompleted:
      count = user_.completed;
      break;
    case sync::myanimelist::kOnHold:
      count = user_.on_hold;
      break;
    case sync::myanimelist::kDropped:
      count = user_.dropped;
      break;
    case sync::myanimelist::kPlanToWatch:
      count = user_.plan_to_watch;
      break;
  }

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

void ListUser::DecreaseItemCount(int status) {
  int count = GetItemCount(status, false) - 1;
  SetItemCount(status, count);
}

void ListUser::IncreaseItemCount(int status) {
  int count = GetItemCount(status, false) + 1;
  SetItemCount(status, count);
}

void ListUser::SetItemCount(int status, int count) {
  switch (status) {
    case sync::myanimelist::kWatching:
      user_.watching = count;
      break;
    case sync::myanimelist::kCompleted:
      user_.completed = count;
      break;
    case sync::myanimelist::kOnHold:
      user_.on_hold = count;
      break;
    case sync::myanimelist::kDropped:
      user_.dropped = count;
      break;
    case sync::myanimelist::kPlanToWatch:
      user_.plan_to_watch = count;
      break;
  }
}

void ListUser::SetId(int id) {
  user_.id = id;
}

void ListUser::SetName(const wstring& name) {
  user_.name = name;
}

void ListUser::SetDaysSpentWatching(const wstring& days_spent_watching) {
  user_.days_spent_watching = days_spent_watching;
}

} // namespace anime