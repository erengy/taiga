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
      rewatching_ep(0) {
}

LocalInformation::LocalInformation()
    : last_aired_episode(0),
      playing(false) {
}

// =============================================================================

void Item::StartWatching(Episode& episode) {
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
  MainDialog.UpdateTip();
  MainDialog.UpdateTitle();
  if (Settings.Account.Update.go_to_nowplaying)
    MainDialog.navigation.SetCurrentPage(SIDEBAR_ITEM_NOWPLAYING);
  
  // Show balloon tip
  if (Settings.Program.Notifications.recognized) {
    Taiga.current_tip_type = TIPTYPE_NOWPLAYING;
    Taskbar.Tip(L"", L"", 0);
    Taskbar.Tip(ReplaceVariables(Settings.Program.Notifications.format, episode).c_str(), 
                L"Now Playing", NIIF_INFO);
  }
  
  // Check folder
  if (GetFolder().empty()) {
    if (episode.folder.empty()) {
      HWND hwnd = MediaPlayers.items[MediaPlayers.index].window_handle;
      episode.folder = MediaPlayers.GetTitleFromProcessHandle(hwnd);
      episode.folder = GetPathOnly(episode.folder);
    }
    if (IsInsideRootFolders(episode.folder)) {
      // Set the folder if only it is under a root folder
      SetFolder(episode.folder, true);
    }
  }

  // Get additional information
  if (GetScore().empty() || GetSynopsis().empty())
    mal::SearchAnime(GetId(), GetTitle());
  
  // Update list
  if (Settings.Account.Update.delay == 0 && !Settings.Account.Update.wait_mp)
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
  MainDialog.UpdateTip();
  MainDialog.UpdateTitle();
  int list_index = AnimeListDialog.GetListIndex(GetId());
  if (list_index > -1) {
    AnimeListDialog.listview.SetItemIcon(list_index, StatusToIcon(GetAiringStatus()));
    AnimeListDialog.listview.RedrawItems(list_index, list_index, true);
  }
}

bool Item::IsUpdateAllowed(const Episode& episode, bool ignore_update_time) {
  if (episode.processed)
    return false;

  if (!ignore_update_time)
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

void Item::UpdateList(Episode& episode) {
  if (!IsUpdateAllowed(episode, false))
    return;

  episode.processed = true;

  if (Settings.Account.Update.ask_to_confirm) {
    ConfirmationQueue.Add(episode);
    ConfirmationQueue.Process();
  } else {
    AddToQueue(episode, true);
  }
}

void Item::AddToQueue(const Episode& episode, bool change_status) {
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
  
  foreach_(i, Aggregator.filter_manager.filters) {
    if (found) break;
    foreach_(j, i->anime_ids) {
      if (*j != anime_id) continue;
      if (found) break;
      foreach_(k, i->conditions) {
        if (k->element == FEED_FILTER_ELEMENT_ANIME_GROUP) {
          groups.push_back(k->value);
          found = true;
        }
      }
    }
  }

  return found;
}

bool SetFansubFilter(int anime_id, const wstring& group_name) {
  // Check existing filters
  foreach_(i, Aggregator.filter_manager.filters) {
    foreach_(j, i->anime_ids) {
      if (*j != anime_id) continue;
      foreach_(k, i->conditions) {
        if (k->element == FEED_FILTER_ELEMENT_ANIME_GROUP) {
          if (group_name.empty()) {
            Aggregator.filter_manager.filters.erase(i);
          } else {
            k->value = group_name;
          }
          return true;
        }
      }
    }
  }

  if (group_name.empty())
    return false;

  // Create new filter
  auto anime_item = AnimeDatabase.FindItem(anime_id);
  Aggregator.filter_manager.AddFilter(
    FEED_FILTER_ACTION_SELECT, FEED_FILTER_MATCH_ANY, true, 
    L"[Fansub] " + anime_item->GetTitle());
  Aggregator.filter_manager.filters.back().AddCondition(
    FEED_FILTER_ELEMENT_ANIME_GROUP, FEED_FILTER_OPERATOR_IS, 
    group_name);
  Aggregator.filter_manager.filters.back().anime_ids.push_back(anime_id);
  return true;
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

bool IsInsideRootFolders(const wstring& path) {
  foreach_c_(root_folder, Settings.Folders.root)
    if (StartsWith(path, *root_folder))
      return true;

  return false;
}

bool UpdateItemFromSettings(int anime_id) {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (!anime_item)
    return false;

  foreach_c_(it, Settings.Anime.items) {
    if (it->id == anime_id) {
      anime_item->SetFolder(it->folder, false);
      anime_item->SetUserSynonyms(it->titles, false);
      return true;
    }
  }

  return false;
}

} // namespace anime