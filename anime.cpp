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

#include "anime.h"
#include "anime_db.h"
#include "anime_episode.h"

#include "announce.h"
#include "common.h"
#include "feed.h"
#include "foreach.h"
#include "history.h"
#include "media.h"
#include "myanimelist.h"
#include "process.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "theme.h"

#include "dlg/dlg_anime_info.h"
#include "dlg/dlg_anime_list.h"
#include "dlg/dlg_feed_filter.h"
#include "dlg/dlg_main.h"

#include "win32/win_taskbar.h"
#include "win32/win_taskdialog.h"

namespace anime {

// =============================================================================

SeriesInformation::SeriesInformation()
    : id(ID_UNKNOWN), 
      type(mal::TYPE_UNKNOWN), 
      episodes(-1), 
      status(mal::STATUS_UNKNOWN) {
}

MyInformation::MyInformation()
    : watched_episodes(0), 
      score(0), 
      status(mal::MYSTATUS_NOTINLIST), 
      rewatching(FALSE), 
      rewatching_ep(0), 
      playing(false) {
}

// =============================================================================

void Item::StartWatching(Episode episode) {
  // Make sure item is in list
  if (!IsInList()) AddtoUserList();

  // Change status
  Taiga.play_status = PLAYSTATUS_PLAYING;
  SetPlaying(true);

  // Update now playing window
  NowPlayingDialog.SetCurrentId(GetId());
  
  // Update anime list window
  int status = GetMyRewatching() ? mal::MYSTATUS_WATCHING : GetMyStatus();
  if (status != mal::MYSTATUS_NOTINLIST) {
    AnimeListDialog.RefreshList(status);
    AnimeListDialog.RefreshTabs(status);
  }
  int list_index = AnimeListDialog.GetListIndex(GetId());
  if (list_index > -1) {
    AnimeListDialog.listview.SetItemIcon(list_index, ICON16_PLAY);
    AnimeListDialog.listview.RedrawItems(list_index, list_index, true);
    AnimeListDialog.listview.EnsureVisible(list_index);
  }

  // Update main window
  MainDialog.EnableSharing();
  MainDialog.UpdateTip();
  MainDialog.UpdateTitle();
  MainDialog.navigation.SetCurrentPage(SIDEBAR_ITEM_NOWPLAYING);
  
  // Show balloon tip
  if (Settings.Program.Balloon.enabled) {
    Taskbar.Tip(L"", L"", 0);
    Taskbar.Tip(ReplaceVariables(Settings.Program.Balloon.format, episode).c_str(), 
      L"Watching:", NIIF_INFO);
  }
  
  // Check folder
  if (GetFolder().empty()) {
    if (episode.folder.empty()) {
      HWND hwnd = MediaPlayers.items[MediaPlayers.index].window_handle;
      episode.folder = MediaPlayers.GetTitleFromProcessHandle(hwnd);
      episode.folder = GetPathOnly(episode.folder);
    }
    if (!episode.folder.empty()) {
      for (size_t i = 0; i < Settings.Folders.root.size(); i++) {
        // Set the folder if only it is under a root folder
        if (StartsWith(episode.folder, Settings.Folders.root[i])) {
          SetFolder(episode.folder, true);
          break;
        }
      }
    }
  }

  // Get additional information
  if (GetScore().empty() || GetSynopsis().empty())
    mal::SearchAnime(GetId(), GetTitle());
  
  // Update list
  if (Settings.Account.Update.time == UPDATE_TIME_INSTANT)
    UpdateList(episode);
}

void Item::EndWatching(Episode episode) {
  // Change status
  Taiga.play_status = PLAYSTATUS_STOPPED;
  SetPlaying(false);
  
  // Announce
  episode.anime_id = GetId();
  Announcer.Do(ANNOUNCE_TO_HTTP, &episode);
  Announcer.Clear(ANNOUNCE_TO_MESSENGER | ANNOUNCE_TO_SKYPE);

  // Update now playing window
  NowPlayingDialog.SetCurrentId(anime::ID_UNKNOWN);
  
  // Update main window
  episode.anime_id = anime::ID_UNKNOWN;
  MainDialog.EnableSharing(false);
  MainDialog.UpdateTip();
  MainDialog.UpdateTitle();
  int list_index = AnimeListDialog.GetListIndex(GetId());
  if (list_index > -1) {
    AnimeListDialog.listview.SetItemIcon(list_index, StatusToIcon(GetAiringStatus()));
    AnimeListDialog.listview.RedrawItems(list_index, list_index, true);
  }
}

bool Item::IsUpdateAllowed(const Episode& episode, bool ignore_update_time) {
  if (Settings.Account.Update.mode == UPDATE_MODE_NONE)
    return false;

  if (!ignore_update_time)
    if (Settings.Account.Update.time != UPDATE_TIME_INSTANT)
      if (Settings.Account.Update.delay > Taiga.ticker_media)
        if (Taiga.ticker_media > -1)
          return false;

  if (GetMyStatus() == mal::MYSTATUS_COMPLETED && GetMyRewatching() == 0)
    return false;
  
  int number = GetEpisodeHigh(episode.number);
  int number_low = GetEpisodeLow(episode.number);
  int last_watched = GetMyLastWatchedEpisode();
    
  if (Settings.Account.Update.out_of_range)
    if (number_low > last_watched + 1 || number < last_watched + 1)
      return false;

  if (!mal::IsValidEpisode(number, last_watched, GetEpisodeCount()))
    return false;

  return true;
}

void Item::UpdateList(Episode episode) {
  if (!IsUpdateAllowed(episode, false)) return;
  
  bool change_status = true;

  if (Settings.Account.Update.mode == UPDATE_MODE_ASK) {
      // Set up dialog
      win32::TaskDialog dlg;
      wstring title = L"Anime title: " + GetTitle();
      dlg.SetWindowTitle(APP_TITLE);
      dlg.SetMainIcon(TD_ICON_INFORMATION);
      dlg.SetMainInstruction(L"Do you want to update your anime list?");
      dlg.SetContent(title.c_str());
      dlg.UseCommandLinks(true);

      // Add buttons
      int number = GetEpisodeHigh(episode.number);
      if (number == 0) number = 1;
      if (GetEpisodeCount() == 1) episode.number = L"1";
      if (GetEpisodeCount() == number) { // Completed
        dlg.AddButton(L"Update and move\nUpdate and set as completed", IDCANCEL);
      } else if (GetMyStatus() != mal::MYSTATUS_WATCHING) { // Watching
        dlg.AddButton(L"Update and move\nUpdate and set as watching", IDCANCEL);
      }
      wstring button = L"Update\nUpdate episode number from " + 
        ToWstr(GetMyLastWatchedEpisode()) + L" to " + ToWstr(number);
      dlg.AddButton(button.c_str(), IDYES);
      dlg.AddButton(L"Cancel\nDon't update anything", IDNO);
  
      // Show dialog
      ActivateWindow(g_hMain);
      dlg.Show(g_hMain);
      int choice = dlg.GetSelectedButtonID();
      if (choice == IDNO) return;
      change_status = choice == IDCANCEL;
  }

  AddToQueue(episode, change_status);
}

void Item::AddToQueue(Episode episode, bool change_status) {
  // Create event item
  EventItem event_item;
  event_item.anime_id = GetId();

  // Set episode number
  event_item.episode = GetEpisodeHigh(episode.number);
  if (*event_item.episode == 0 || GetEpisodeCount() == 1) event_item.episode = 1;
  
  // Set start/finish date
  if (*event_item.episode == 1 && !mal::IsValidDate(GetMyDate(DATE_START)))
    event_item.date_start = mal::TranslateDateForApi(::GetDate());
  if (*event_item.episode == GetEpisodeCount() && !mal::IsValidDate(GetMyDate(DATE_END)))
    event_item.date_finish = mal::TranslateDateForApi(::GetDate());

  // Set update mode
  if (GetMyStatus() == mal::MYSTATUS_NOTINLIST) {
    event_item.mode = HTTP_MAL_AnimeAdd;
  } else if (change_status) {
    event_item.mode = HTTP_MAL_AnimeEdit;
  } else {
    event_item.mode = HTTP_MAL_AnimeUpdate;
  }

  if (change_status) {
    // Move to completed
    if (GetEpisodeCount() == *event_item.episode) {
      event_item.status = mal::MYSTATUS_COMPLETED;
      if (GetMyRewatching()) {
        event_item.enable_rewatching = FALSE;
        //event_item.times_rewatched++; // TODO: Enable when MAL adds to API
      }
    // Move to watching
    } else if (GetMyStatus() != mal::MYSTATUS_WATCHING || *event_item.episode == 1) {
      if (!GetMyRewatching()) {
        event_item.status = mal::MYSTATUS_WATCHING;
      }
    }
  }

  // Add event to queue
  History.queue.Add(event_item);
}

// =============================================================================

bool GetFansubFilter(int anime_id, vector<wstring>& groups) {
  bool found = false;
  for (auto i = Aggregator.filter_manager.filters.begin(); 
       i != Aggregator.filter_manager.filters.end(); ++i) {
    if (found) break;
    for (auto j = i->anime_ids.begin(); j != i->anime_ids.end(); ++j) {
      if (*j != anime_id) continue;
      if (found) break;
      for (auto k = i->conditions.begin(); k != i->conditions.end(); ++k) {
        if (k->element != FEED_FILTER_ELEMENT_ANIME_GROUP) continue;
        groups.push_back(k->value);
        found = true;
      }
    }
  }
  return found;
}

bool SetFansubFilter(int anime_id, const wstring& group_name) {
  FeedFilter* filter = nullptr;

  for (auto i = Aggregator.filter_manager.filters.begin(); 
       i != Aggregator.filter_manager.filters.end(); ++i) {
    if (filter) break;
    for (auto j = i->anime_ids.begin(); j != i->anime_ids.end(); ++j) {
      if (*j != anime_id) continue;
      for (auto k = i->conditions.begin(); k != i->conditions.end(); ++k) {
        if (k->element != FEED_FILTER_ELEMENT_ANIME_GROUP) continue;
        filter = &(*i); break;
      }
    }
  }
  
  if (filter) {
    FeedFilterDialog.filter = *filter;
  } else {
    FeedFilterDialog.filter.Reset();
    FeedFilterDialog.filter.name = L"[Fansub] " + AnimeDatabase.FindItem(anime_id)->GetTitle();
    FeedFilterDialog.filter.match = FEED_FILTER_MATCH_ANY;
    FeedFilterDialog.filter.action = FEED_FILTER_ACTION_SELECT;
    FeedFilterDialog.filter.anime_ids.push_back(anime_id);
    FeedFilterDialog.filter.AddCondition(FEED_FILTER_ELEMENT_ANIME_GROUP, 
      FEED_FILTER_OPERATOR_IS, group_name);
  }

  ExecuteAction(L"TorrentAddFilter", TRUE, reinterpret_cast<LPARAM>(g_hMain));
  
  if (!FeedFilterDialog.filter.conditions.empty()) {
    if (filter) {
      *filter = FeedFilterDialog.filter;
    } else {
      Aggregator.filter_manager.filters.push_back(FeedFilterDialog.filter);
    }
    return true;
  } else {
    return false;
  }
}

wstring GetImagePath(int anime_id) {
  wstring path = Taiga.GetDataPath() + L"db\\image\\";
  if (anime_id > 0) path += ToWstr(anime_id) + L".jpg";
  return path;
}

void GetUpcomingTitles(vector<int>& anime_ids) {
  foreach_c_(item, AnimeDatabase.items) {
    const anime::Item& anime_item = item->second;
    
    const Date& date_start = anime_item.GetDate(anime::DATE_START);
    const Date& date_now = GetDateJapan();

    if (!date_start.year || !date_start.month || !date_start.day)
      continue;

    if (date_start > date_now &&
        ToDayCount(date_start) < ToDayCount(date_now) + 7) { // Same week
      anime_ids.push_back(anime_item.GetId());
    }
  }
}

} // namespace anime