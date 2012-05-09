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

#include "event.h"

#include "anime_db.h"
#include "announce.h"
#include "common.h"
#include "debug.h"
#include "http.h"
#include "myanimelist.h"
#include "resource.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"

#include "dlg/dlg_event.h"
#include "dlg/dlg_main.h"

#include "win32/win_taskdialog.h"

class EventQueue EventQueue;

// =============================================================================

EventItem::EventItem()
    : anime_id(0), 
      enabled(true), 
      mode(0) {
}

EventList::EventList()
    : index(0) {
}

void EventList::Add(EventItem& item) {
  auto anime = AnimeDatabase.FindItem(item.anime_id);
  
  // Validate values
  if (anime && item.mode != HTTP_MAL_AnimeAdd) {
    if (item.episode)
      if (anime->GetMyLastWatchedEpisode() == *item.episode || *item.episode < 0)
        item.episode.Reset();
    if (item.score)
      if (anime->GetMyScore() == *item.score || *item.score < 0 || *item.score > 10)
        item.score.Reset();
    if (item.status)
      if (anime->GetMyStatus() == *item.status || *item.status < 1 || *item.status == 5 || *item.status > 6)
        item.status.Reset();
    if (item.enable_rewatching)
      if (anime->GetMyRewatching() == *item.enable_rewatching)
        item.enable_rewatching.Reset();
    if (item.tags)
      if (anime->GetMyTags() == *item.tags)
        item.tags.Reset();
    if (item.date_start)
      if (anime->GetDate(anime::DATE_START) == mal::TranslateDateFromApi(*item.date_start))
        item.date_start.Reset();
    if (item.date_finish)
      if (anime->GetDate(anime::DATE_END) == mal::TranslateDateFromApi(*item.date_finish))
        item.date_finish.Reset();
  }
  switch (item.mode) {
    case HTTP_MAL_AnimeEdit:
      if (!item.episode && 
          !item.score && 
          !item.status && 
          !item.enable_rewatching && 
          !item.tags && 
          !item.date_start && 
          !item.date_finish) return;
      break;
    case HTTP_MAL_AnimeUpdate:
      if (!item.episode) return;
      break;
    case HTTP_MAL_ScoreUpdate:
      if (!item.score) return;
      break;
    case HTTP_MAL_StatusUpdate:
      if (!item.status) return;
      break;
    case HTTP_MAL_TagUpdate:
      if (!item.tags) return;
      break;
  }

  // Edit previous item with the same ID...
  bool add_new_item = true;
  if (!EventQueue.updating) {
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
      if (it->anime_id == item.anime_id) {
        if (it->mode != HTTP_MAL_AnimeAdd && it->mode != HTTP_MAL_AnimeDelete) {
          if (!item.episode || (!it->episode && it == items.rbegin())) {
            if (item.episode) it->episode = *item.episode;
            if (item.score) it->score = *item.score;
            if (item.status) it->status = *item.status;
            if (item.enable_rewatching) it->enable_rewatching = *item.enable_rewatching;
            if (item.tags) it->tags = *item.tags;
            if (item.date_start) it->date_start = *item.date_start;
            if (item.date_finish) it->date_finish = *item.date_finish;
            add_new_item = false;
          }
          if (!add_new_item) {
            it->mode = HTTP_MAL_AnimeEdit;
            it->time = (wstring)GetDate() + L" " + GetTime();
          }
          break;
        }
      }
    }
  }
  // ...or add a new one
  if (add_new_item) {
    if (item.time.empty()) item.time = (wstring)GetDate() + L" " + GetTime();
    items.push_back(item);
  }

  if (anime) {
    // Announce
    if (Taiga.logged_in && item.episode) {
      anime::Episode episode;
      episode.anime_id = anime->GetId();
      episode.number = ToWstr(*item.episode);
      Taiga.play_status = PLAYSTATUS_UPDATED;
      Announcer.Do(ANNOUNCE_TO_HTTP | ANNOUNCE_TO_TWITTER, &episode);
    }

    // Check new episode
    if (item.episode) {
      anime->SetNewEpisodePath(L"");
      anime->CheckEpisodes(0);
    }
    
    // Refresh event window
    EventDialog.RefreshList();
    
    // Refresh main window
    MainDialog.RefreshList();
    MainDialog.RefreshTabs();
    
    // Change status
    if (!Taiga.logged_in) {
      MainDialog.ChangeStatus(L"Item added to the event queue. (" + 
        anime->GetTitle() + L")");
    }

    // Update
    Check();
  }
}

void EventList::Check() {
  // Check
  if (items.empty()) {
    return;
  }
  if (!items[index].enabled) {
    debug::Print(L"EventList::Check :: Item is disabled, removing...\n");
    Remove(index);
    Check();
    return;
  }
  if (!Taiga.logged_in) {
    items[index].reason = L"Not logged in";
    return;
  }
  
  // Compare ID with anime list
  auto anime_item = AnimeDatabase.FindItem(items[index].anime_id);
  if (!anime_item) {
    debug::Print(L"EventList::Check :: Item not found in list, removing...\n");
    Remove(index);
    Check();
    return;
  }
  
  // Update
  EventQueue.updating = true;
  MainDialog.ChangeStatus(L"Updating list...");
  mal::AnimeValues* anime_values = reinterpret_cast<mal::AnimeValues*>(&items[index]);
  mal::Update(*anime_values, items[index].anime_id, items[index].mode);
}

void EventList::Clear() {
  items.clear();
  index = 0;
}

EventItem* EventList::FindItem(int anime_id, int search_mode) {
  for (auto it = items.rbegin(); it != items.rend(); ++it) {
    if (it->anime_id == anime_id) {
      switch (search_mode) {
        // Date
        case EVENT_SEARCH_DATE_START:
          if (it->date_start) return &(*it);
          break;
        case EVENT_SEARCH_DATE_END:
          if (it->date_finish) return &(*it);
          break;
        // Episode
        case EVENT_SEARCH_EPISODE:
          if (it->episode) return &(*it);
          break;
        // Re-watching
        case EVENT_SEARCH_REWATCH:
          if (it->enable_rewatching) return &(*it);
          break;
        // Score
        case EVENT_SEARCH_SCORE:
          if (it->score) return &(*it);
          break;
        // Status
        case EVENT_SEARCH_STATUS:
          if (it->status) return &(*it);
          break;
        // Tags
        case EVENT_SEARCH_TAGS:
          if (it->tags) return &(*it);
          break;
        // Default
        default:
          return &(*it);
      }
    }
  }
  return nullptr;
}

void EventList::Remove(unsigned int index, bool refresh) {
  // Remove item
  if (index < items.size()) {
    items.erase(items.begin() + index);
    if (refresh) EventDialog.RefreshList();
  }
}

// =============================================================================

EventQueue::EventQueue()
    : updating(false) {
}

void EventQueue::Add(EventItem& item, bool save, wstring user) {
  EventList* event_list = FindList(user);
  if (event_list == nullptr) {
    list.resize(list.size() + 1);
    list.back().user = user.empty() ? Settings.Account.MAL.user : user;
    event_list = &list.back();
  }
  event_list->Add(item);
  if (save) Settings.Save();
}

void EventQueue::Check() {
  EventList* event_list = FindList();
  if (event_list) event_list->Check();
}

void EventQueue::Clear(bool save) {
  EventList* event_list = FindList();
  if (event_list) {
    event_list->Clear();
    if (save) Settings.Save();
  }
}

EventItem* EventQueue::FindItem(int anime_id, int search_mode) {
  EventList* event_list = FindList();
  if (event_list) return event_list->FindItem(anime_id, search_mode);
  return nullptr;
}

EventList* EventQueue::FindList(wstring user) {
  if (user.empty()) user = Settings.Account.MAL.user;
  for (unsigned int i = 0; i < list.size(); i++) {
    if (list[i].user == user) return &list[i];
  }
  return nullptr;
}

int EventQueue::GetItemCount() {
  EventList* event_list = FindList();
  if (event_list) return event_list->items.size();
  return 0;
}

bool EventQueue::IsEmpty() {
  for (unsigned int i = 0; i < list.size(); i++) {
    if (list[i].items.size() > 0) return false;
  }
  return true;
}

void EventQueue::Remove(int index, bool save, bool refresh) {
  EventList* event_list = FindList();
  if (event_list) {
    if (index > -1) {
      event_list->Remove(static_cast<unsigned int>(index), refresh);
    } else {
      event_list->Remove(event_list->index, refresh);
    }
    if (save) Settings.Save();
  }
}

void EventQueue::Show() {
  if (GetItemCount() == 0) {
    win32::TaskDialog dlg(L"Previously on Taiga...", TD_ICON_INFORMATION);
    dlg.SetMainInstruction(L"There are no events in the queue.");
    dlg.AddButton(L"OK", IDOK);
    dlg.Show(g_hMain);
    return;
  }
  
  if (EventDialog.IsWindow()) {
    EventDialog.Show();
  } else {
    EventDialog.Create(IDD_EVENT, g_hMain, false);
  }
}