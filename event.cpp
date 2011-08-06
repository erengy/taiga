/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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
#include "animelist.h"
#include "common.h"
#include "dlg/dlg_event.h"
#include "dlg/dlg_main.h"
#include "event.h"
#include "http.h"
#include "myanimelist.h"
#include "resource.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "win32/win_taskdialog.h"

CEventQueue EventQueue;

// =============================================================================

CEventList::CEventList() : 
  Index(0)
{
}

void CEventList::Add(CEventItem& item) {
  CAnime* anime = AnimeList.FindItem(item.AnimeId);
  
  // Validate values
  if (anime && item.Mode != HTTP_MAL_AnimeAdd) {
    if (anime->GetLastWatchedEpisode() == item.episode || item.episode < 0) {
      item.episode = -1;
    }
    if (anime->GetScore() == item.score || item.score < 0 || item.score > 10) {
      item.score = -1;
    }
    if (anime->GetStatus() == item.status || item.status < 1 || item.status == 5 || item.status > 6) {
      item.status = -1;
    }
    if (anime->GetRewatching() == item.enable_rewatching) {
      item.enable_rewatching = -1;
    }
    if (anime->GetTags() == item.tags) {
      item.tags = EMPTY_STR;
    }
  }
  switch (item.Mode) {
    case HTTP_MAL_AnimeEdit:
      if (item.episode == -1 && item.score == -1 && item.status == -1 && item.enable_rewatching == -1 && item.tags == EMPTY_STR) return;
      break;
    case HTTP_MAL_AnimeUpdate:
      if (item.episode == -1) return;
      break;
    case HTTP_MAL_ScoreUpdate:
      if (item.score == -1) return;
      break;
    case HTTP_MAL_StatusUpdate:
      if (item.status == -1) return;
      break;
    case HTTP_MAL_TagUpdate:
      if (item.tags == EMPTY_STR) return;
      break;
  }

  // Compare with previous items in buffer
  for (unsigned int i = 0; i < Items.size(); i++) {
    if (Items[i].AnimeId == item.AnimeId && Items[i].Mode == item.Mode) { 
      #define COMPAREITEMS(x) item.x == Items[i].x
      #define COMPAREITEMSI(x) (item.x > -1 && Items[i].x > -1 && item.x != Items[i].x)
      #define COMPAREITEMSS(x) (item.x != EMPTY_STR && Items[i].x != EMPTY_STR && item.x != Items[i].x)
      if (COMPAREITEMSI(score) || COMPAREITEMSI(status) || COMPAREITEMSI(enable_rewatching) || COMPAREITEMSS(tags) ||
         (COMPAREITEMS(episode) && COMPAREITEMS(score) && COMPAREITEMS(status) && COMPAREITEMS(enable_rewatching) &&  COMPAREITEMS(tags))) {
           if (!EventQueue.UpdateInProgress) {
             Remove(i);
             break;
           }
      }
      #undef COMPAREITEMS
      #undef COMPAREITEMSI
      #undef COMPAREITEMSS
    }
  }

  // Add new item
  if (item.Time.empty()) item.Time = GetDate() + L" " + GetTime();
  Items.push_back(item);

  if (anime) {
    // Announce
    if (Taiga.LoggedIn && Taiga.UpdatesEnabled && item.episode > 0) {
      CEpisode episode;
      episode.AnimeId = anime->Series_ID;
      episode.Number = ToWSTR(item.episode);
      Taiga.PlayStatus = PLAYSTATUS_UPDATED;
      ExecuteAction(L"AnnounceToHTTP", TRUE, reinterpret_cast<LPARAM>(&episode));
      ExecuteAction(L"AnnounceToTwitter", 0, reinterpret_cast<LPARAM>(&episode));
    }

    // Check new episode
    if (item.episode > -1) {
      anime->NewEps = false;
      anime->CheckEpisodes(0);
    }
    
    // Refresh event window
    EventWindow.RefreshList();
    
    // Refresh main window
    MainWindow.RefreshList();
    MainWindow.RefreshTabs();
    
    // Change status
    if (!Taiga.LoggedIn) {
      MainWindow.ChangeStatus(L"Item added to the event queue. (" + anime->Series_Title + L")");
    }

    // Update
    Check();
  }
}

void CEventList::Check() {
  // Check
  if (Items.empty()) return;
  if (!Taiga.UpdatesEnabled) {
    Items[Index].Reason = L"Updates are disabled";
    return;
  }
  if (!Taiga.LoggedIn) {
    Items[Index].Reason = L"Not logged in";
    return;
  }
  
  // Compare ID with anime list
  CAnime* anime = AnimeList.FindItem(Items[Index].AnimeId);
  if (!anime) {
    DEBUG_PRINT(L"CEventList::Check :: Item not found in list, removing...\n");
    Remove(Index);
    Check();
    return;
  }
  
  // Update
  EventQueue.UpdateInProgress = true;
  MainWindow.ChangeStatus(L"Updating list...");
  CMALAnimeValues* anime_values = reinterpret_cast<CMALAnimeValues*>(&Items[Index]);
  MAL.Update(*anime_values, Items[Index].AnimeId, Items[Index].Mode);
}

void CEventList::Clear() {
  Items.clear();
  Index = 0;
}

void CEventList::Remove(unsigned int index) {
  // Remove item
  if (index < Items.size()) {
    Items.erase(Items.begin() + index);
    EventWindow.RefreshList();
  }
}

CEventItem* CEventList::SearchItem(int anime_id, int search_mode) {
  for (int i = static_cast<int>(Items.size()) - 1; i >= 0; i--) {
    if (Items[i].AnimeId == anime_id) {
      switch (search_mode) {
        // Episode
        case EVENT_SEARCH_EPISODE:
          if (Items[i].episode > -1)
            return &Items[i];
          break;
        // Rewatching
        case EVENT_SEARCH_REWATCH:
          if (Items[i].enable_rewatching > -1)
            return &Items[i];
          break;
        // Score
        case EVENT_SEARCH_SCORE:
          if (Items[i].score > -1)
            return &Items[i];
          break;
        // Status
        case EVENT_SEARCH_STATUS:
          if (Items[i].status > -1)
            return &Items[i];
          break;
        // Tags
        case EVENT_SEARCH_TAGS:
          if (Items[i].tags != EMPTY_STR)
            return &Items[i];
          break;
        // Default
        default:
          return &Items[i];
      }
    }
  }
  return nullptr;
}

// =============================================================================

CEventQueue::CEventQueue() :
  UpdateInProgress(false)
{
}

void CEventQueue::Add(CEventItem& item, bool save, wstring user) {
  int user_index = GetUserIndex(user);
  if (user_index == -1) {
    List.resize(List.size() + 1);
    List.back().User = user.empty() ? Settings.Account.MAL.User : user;
    user_index = List.size() - 1;
  }
  List[user_index].Add(item);
  if (save) Settings.Write();
}

void CEventQueue::Check() {
  int user_index = GetUserIndex();
  if (user_index > -1) List[user_index].Check();
}

void CEventQueue::Clear(bool save) {
  int user_index = GetUserIndex();
  if (user_index > -1) {
    List[user_index].Clear();
    if (save) Settings.Write();
  }
}

int CEventQueue::GetItemCount() {
  int user_index = GetUserIndex();
  if (user_index > -1) return List[user_index].Items.size();
  return 0;
}

int CEventQueue::GetUserIndex(wstring user) {
  if (user.empty()) user = Settings.Account.MAL.User;
  for (unsigned int i = 0; i < List.size(); i++) {
    if (List[i].User == user) return i;
  }
  return -1;
}

bool CEventQueue::IsEmpty() {
  for (unsigned int i = 0; i < List.size(); i++) {
    if (List[i].Items.size() > 0) return false;
  }
  return true;
}

void CEventQueue::Remove(int index, bool save) {
  int user_index = GetUserIndex();
  if (user_index > -1) {
    if (index > -1) {
      List[user_index].Remove(static_cast<unsigned int>(index));
    } else {
      List[user_index].Remove(List[user_index].Index);
    }
    if (save) Settings.Write();
  }
}

CEventItem* CEventQueue::SearchItem(int anime_id, int search_mode) {
  int user_index = GetUserIndex();
  if (user_index > -1) return List[user_index].SearchItem(anime_id, search_mode);
  return NULL;
}

void CEventQueue::Show() {
  if (GetItemCount() == 0) {
    CTaskDialog dlg(L"Previously on Taiga...", TD_ICON_INFORMATION);
    dlg.SetMainInstruction(L"There are no events in the queue.");
    dlg.AddButton(L"OK", IDOK);
    dlg.Show(g_hMain);
    return;
  }
  
  if (EventWindow.IsWindow()) {
    EventWindow.Show();
  } else {
    EventWindow.Create(IDD_EVENT, g_hMain, false);
  }
}