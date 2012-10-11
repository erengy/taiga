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

#include "std.h"

#include "anime_db.h"
#include "anime_user.h"

#include "history.h"
#include "http.h"
#include "myanimelist.h"
#include "string.h"

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
    case mal::MYSTATUS_WATCHING:
      count = user_.watching;
      break;
    case mal::MYSTATUS_COMPLETED:
      count = user_.completed;
      break;
    case mal::MYSTATUS_ONHOLD:
      count = user_.on_hold;
      break;
    case mal::MYSTATUS_DROPPED:
      count = user_.dropped;
      break;
    case mal::MYSTATUS_PLANTOWATCH:
      count = user_.plan_to_watch;
      break;
  }

  // Search event queue for status changes
  EventList* event_list = History.queue.FindList();
  if (check_events && event_list) {
    for (auto it = event_list->items.begin(); it != event_list->items.end(); ++it) {
      if (it->mode == HTTP_MAL_AnimeAdd) continue;
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

void ListUser::DecreaseItemCount(int status, bool save_list) {
  int count = GetItemCount(status, false) - 1;
  SetItemCount(status, count, save_list);
}

void ListUser::IncreaseItemCount(int status, bool save_list) {
  int count = GetItemCount(status, false) + 1;
  SetItemCount(status, count, save_list);
}

void ListUser::SetItemCount(int status, int count, bool save_list) {
  switch (status) {
    case mal::MYSTATUS_WATCHING:
      user_.watching = count;
      if (save_list) database_->SaveList(
        -1, L"user_watching", ToWstr(user_.watching), EDIT_USER);
      break;
    case mal::MYSTATUS_COMPLETED:
      user_.completed = count;
      if (save_list) database_->SaveList(
        -1, L"user_completed", ToWstr(user_.completed), EDIT_USER);
      break;
    case mal::MYSTATUS_ONHOLD:
      user_.on_hold = count;
      if (save_list) database_->SaveList(
        -1, L"user_onhold", ToWstr(user_.on_hold), EDIT_USER);
      break;
    case mal::MYSTATUS_DROPPED:
      user_.dropped = count;
      if (save_list) database_->SaveList(
        -1, L"user_dropped", ToWstr(user_.dropped), EDIT_USER);
      break;
    case mal::MYSTATUS_PLANTOWATCH:
      user_.plan_to_watch = count;
      if (save_list) database_->SaveList(
        -1, L"user_plantowatch", ToWstr(user_.plan_to_watch), EDIT_USER);
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