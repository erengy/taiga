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
  // Validate values
  if (item.AnimeIndex > 0 && item.AnimeIndex <= AnimeList.Count && item.Mode != HTTP_MAL_AnimeAdd) {
    if (AnimeList.Item[item.AnimeIndex].GetLastWatchedEpisode() == item.episode || item.episode < 0) {
      item.episode = -1;
    }
    if (AnimeList.Item[item.AnimeIndex].GetScore() == item.score || item.score < 0 || item.score > 10) {
      item.score = -1;
    }
    if (AnimeList.Item[item.AnimeIndex].GetStatus() == item.status || item.status < 1 || item.status == 5 || item.status > 6) {
      item.status = -1;
    }
    if (AnimeList.Item[item.AnimeIndex].GetRewatching() == item.enable_rewatching) {
      item.enable_rewatching = -1;
    }
    if (AnimeList.Item[item.AnimeIndex].GetTags() == item.tags) {
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
  for (unsigned int i = 0; i < Item.size(); i++) {
    if (Item[i].AnimeIndex == item.AnimeIndex && Item[i].Mode == item.Mode) { 
      #define COMPAREITEMS(x) item.x == Item[i].x
      #define COMPAREITEMSI(x) (item.x > -1 && Item[i].x > -1 && item.x != Item[i].x)
      #define COMPAREITEMSS(x) (item.x != EMPTY_STR && Item[i].x != EMPTY_STR && item.x != Item[i].x)
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
  if (!item.AnimeID) item.AnimeID = AnimeList.Item[item.AnimeIndex].Series_ID;
  if (item.Time.empty()) item.Time = GetDate() + L" " + GetTime();
  Item.push_back(item);

  if (item.AnimeIndex > 0 && item.AnimeIndex <= AnimeList.Count) {
    // Announce
    if (Taiga.LoggedIn && Taiga.UpdatesEnabled && item.episode > 0) {
      CEpisode episode;
      episode.Index = item.AnimeIndex;
      episode.Number = ToWSTR(item.episode);
      Taiga.PlayStatus = PLAYSTATUS_UPDATED;
      ExecuteAction(L"AnnounceToHTTP", TRUE, reinterpret_cast<LPARAM>(&episode));
      ExecuteAction(L"AnnounceToTwitter", 0, reinterpret_cast<LPARAM>(&episode));
    }

    // Check new episode
    if (item.episode > -1) {
      AnimeList.Item[item.AnimeIndex].NewEps = false;
      AnimeList.Item[item.AnimeIndex].CheckEpisodes(0);
    }
    
    // Refresh event window
    EventWindow.RefreshList();
    
    // Refresh main window
    MainWindow.RefreshList();
    MainWindow.RefreshTabs();
    
    // Change status
    if (!Taiga.LoggedIn) {
      MainWindow.ChangeStatus(L"Item added to the event queue. (" + 
        AnimeList.Item[item.AnimeIndex].Series_Title + L")");
    }

    // Update
    Check();
  }
}

void CEventList::Check() {
  // Check
  if (Item.empty()) return;
  if (!Taiga.UpdatesEnabled) {
    Item[Index].Reason = L"Updates are disabled";
    return;
  }
  if (!Taiga.LoggedIn) {
    Item[Index].Reason = L"Not logged in";
    return;
  }
  
  // Compare ID with anime list
  if (Index > 0 && AnimeList.Item[Item[Index].AnimeIndex].Series_ID != Item[Index].AnimeID) {
    for (int i = 1; i <= AnimeList.Count; i++) {
      if (AnimeList.Item[i].Series_ID == Item[Index].AnimeID) {
        Item[Index].AnimeIndex = i;
        break;
      }
      if (i == AnimeList.Count) {
        Remove(Index);
        Check();
        return;
      }
    }
  }
  
  // Update
  EventQueue.UpdateInProgress = true;
  MainWindow.ChangeStatus(L"Updating list...");
  CMALAnimeValues* anime = reinterpret_cast<CMALAnimeValues*>(&Item[Index]);
  MAL.Update(*anime, Item[Index].AnimeIndex, Item[Index].AnimeID, Item[Index].Mode);
}

void CEventList::Clear() {
  Item.clear();
  Index = 0;
}

void CEventList::Remove(unsigned int index) {
  // Remove item
  if (index < Item.size()) {
    Item.erase(Item.begin() + index);
    EventWindow.RefreshList();
  }
}

CEventItem* CEventList::SearchItem(int anime_index, int search_mode) {
  for (int i = static_cast<int>(Item.size()) - 1; i >= 0; i--) {
    if (Item[i].AnimeIndex == anime_index) {
      switch (search_mode) {
        // Episode
        case EVENT_SEARCH_EPISODE:
          if (Item[i].episode > -1)
            return &Item[i];
          break;
        // Rewatching
        case EVENT_SEARCH_REWATCH:
          if (Item[i].enable_rewatching > -1)
            return &Item[i];
          break;
        // Score
        case EVENT_SEARCH_SCORE:
          if (Item[i].score > -1)
            return &Item[i];
          break;
        // Status
        case EVENT_SEARCH_STATUS:
          if (Item[i].status > -1)
            return &Item[i];
          break;
        // Tags
        case EVENT_SEARCH_TAGS:
          if (Item[i].tags != EMPTY_STR)
            return &Item[i];
          break;
        // Default
        default:
          return &Item[i];
      }
    }
  }
  return NULL;
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
  if (user_index > -1) return List[user_index].Item.size();
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
    if (List[i].Item.size() > 0) return false;
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

CEventItem* CEventQueue::SearchItem(int anime_index, int search_mode) {
  int user_index = GetUserIndex();
  if (user_index > -1) return List[user_index].SearchItem(anime_index, search_mode);
  return NULL;
}

void CEventQueue::Show() {
  if (GetItemCount() == 0) {
    CTaskDialog dlg;
    dlg.SetWindowTitle(L"Previously on Taiga...");
    dlg.SetMainIcon(TD_ICON_INFORMATION);
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