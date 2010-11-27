/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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
#include "ui/ui_taskdialog.h"

CEventQueue EventQueue;

// =============================================================================

CEventList::CEventList() : 
  Index(0)
{
}

void CEventList::Add(int index, int id, int episode, int score, int status, wstring tags, wstring time, int mode) {
  // Validate values
  if (index > 0 && index <= AnimeList.Count) {
    if (AnimeList.Item[index].My_WatchedEpisodes == episode || episode < 0) {
      episode = -1;
    }
    if (AnimeList.Item[index].My_Score == score || score < 0 || score > 10) {
      score = -1;
    }
    if (AnimeList.Item[index].My_Status == status || status < 1 || status == 5 || status > 6) {
      status = -1;
    }
    if (AnimeList.Item[index].My_Tags == tags) {
      tags = L"%empty%";
    }
  }
  switch (mode) {
    case HTTP_MAL_AnimeEdit:
      if (episode == -1 && score == -1 && status == -1 && tags == L"%empty%") return;
      break;
    case HTTP_MAL_AnimeUpdate:
      if (episode == -1) return;
      break;
    case HTTP_MAL_ScoreUpdate:
      if (score == -1) return;
      break;
    case HTTP_MAL_StatusUpdate:
      if (status == -1) return;
      break;
    case HTTP_MAL_TagUpdate:
      if (tags == L"%empty%") return;
      break;
  }

  // Compare with previous items in buffer
  for (unsigned int i = 0; i < Item.size(); i++) {
    if (Item[i].Index == index && Item[i].Mode == mode) { 
      if (episode < Item[i].Episode) {
        return;
      }
      if (score > -1 && Item[i].Score > -1  && Item[i].Score != score) {
        Remove(i); break;
      }
      if (status > -1 && Item[i].Status > -1 && Item[i].Status != status) {
        Remove(i); break;
      }
      if (tags != L"%empty%" && Item[i].Tags != L"%empty%" && Item[i].Tags != tags) {
        Remove(i); break;
      }
      if (Item[i].Episode == episode && Item[i].Score == score && 
          Item[i].Status  == status  && Item[i].Tags  == tags) {
            return;
      }
    }
  }

  // Add new item
  Item.resize(Item.size() + 1);
  Item.back().ID      = id > 0 ? id : AnimeList.Item[index].Series_ID;
  Item.back().Index   = index;
  Item.back().Episode = episode;
  Item.back().Score   = score;
  Item.back().Status  = status;
  Item.back().Tags    = tags;
  Item.back().Mode    = mode;
  Item.back().Time    = time.empty() ? GetDate() + L" " + GetTime() : time;

  // Announce
  if (Taiga.LoggedIn && Taiga.UpdatesEnabled && episode > 0 && index > 0) {
    CEpisode temp_episode;
    temp_episode.Index = index;
    temp_episode.Number = ToWSTR(episode);
    Taiga.PlayStatus = PLAYSTATUS_UPDATED;
    ExecuteAction(L"AnnounceToHTTP", TRUE, reinterpret_cast<LPARAM>(&temp_episode));
    ExecuteAction(L"AnnounceToTwitter", 0, reinterpret_cast<LPARAM>(&temp_episode));
  }
  
  // Change status
  if (index > 0 && index <= AnimeList.Count) {
    MainWindow.ChangeStatus(L"Item added to the event queue. (" + AnimeList.Item[index].Series_Title + L")");
  }
  EventWindow.RefreshList();
  
  // Update
  Check();
}

void CEventList::Check() {
  // Check
  if (Item.empty()) return;
  if (!Taiga.UpdatesEnabled) {
    Item[Index].Reason = L"Updates are disabled.";
    return;
  }
  if (!Taiga.LoggedIn) {
    Item[Index].Reason = L"Not logged in.";
    return;
  }
  
  // Compare ID with anime list
  if (Index > 0 && AnimeList.Item[Item[Index].Index].Series_ID != Item[Index].ID) {
    for (int i = 1; i <= AnimeList.Count; i++) {
      if (AnimeList.Item[i].Series_ID == Item[Index].ID) {
        Item[Index].Index = i;
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
  MAL.Update(Item[Index].Index, Item[Index].ID, Item[Index].Episode, 
    Item[Index].Score, Item[Index].Status, Item[Index].Tags, 
    Item[Index].Mode);
}

void CEventList::Clear() {
  Item.clear();
  Index = 0;
}

int CEventList::GetLastWatchedEpisode(int index) {
  int value = 0;
  for (unsigned int i = 0; i < Item.size(); i++) {
    if (Item[i].Index == index)
      if (Item[i].Episode > value)
        value = Item[i].Episode;
  }
  return value;
}

void CEventList::Remove(unsigned int index) {
  // Remove item
  if (index < Item.size()) {
    Item.erase(Item.begin() + index);
    EventWindow.RefreshList();
  }
}

// =============================================================================

CEventQueue::CEventQueue() :
  UpdateInProgress(false)
{
}

void CEventQueue::Add(wstring user, int index, int id, int episode, int score, int status, wstring tags, wstring time, int mode) {
  int user_index = GetUserIndex(user);
  if (user_index == -1) {
    List.resize(List.size() + 1);
    List.back().User = user.empty() ? Settings.Account.MAL.User : user;
    user_index = List.size() - 1;
  }
  List[user_index].Add(index, id, episode, score, status, tags, time, mode);
}

void CEventQueue::Check() {
  int user_index = GetUserIndex();
  if (user_index > -1) List[user_index].Check();
}

void CEventQueue::Clear() {
  int user_index = GetUserIndex();
  if (user_index > -1) List[user_index].Clear();
}

int CEventQueue::GetItemCount() {
  int user_index = GetUserIndex();
  if (user_index > -1) return List[user_index].Item.size();
  return 0;
}

int CEventQueue::GetLastWatchedEpisode(int index) {
  int user_index = GetUserIndex();
  if (user_index > -1) return List[user_index].GetLastWatchedEpisode(index);
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

void CEventQueue::Remove(int index) {
  int user_index = GetUserIndex();
  if (user_index > -1) {
    if (index > -1) {
      List[user_index].Remove(static_cast<unsigned int>(index));
    } else {
      List[user_index].Remove(List[user_index].Index);
    }
  }
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