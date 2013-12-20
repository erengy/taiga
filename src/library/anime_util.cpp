/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

#include "anime.h"
#include "anime_db.h"
#include "anime_episode.h"
#include "anime_util.h"

#include "taiga/announce.h"
#include "base/common.h"
#include "base/file.h"
#include "base/logger.h"
#include "track/feed.h"
#include "base/foreach.h"
#include "history.h"
#include "track/media.h"
#include "sync/sync.h"
#include "base/process.h"
#include "taiga/settings.h"
#include "base/string.h"
#include "taiga/path.h"
#include "taiga/script.h"
#include "taiga/taiga.h"
#include "ui/theme.h"

#include "ui/dlg/dlg_anime_info.h"
#include "ui/dlg/dlg_anime_list.h"
#include "ui/dlg/dlg_main.h"

#include "win/win_taskbar.h"
#include "win/win_taskdialog.h"

namespace anime {

bool IsAiredYet(const Item& item) {
  if (item.GetAiringStatus(false) != kNotYetAired)
    return true;

  if (!IsValidDate(item.GetDateStart()))
    return false;
  
  Date date_japan = GetDateJapan();
  Date date_start = item.GetDateStart();
  
  // Assume the worst case
  if (!date_start.month)
    date_start.month = 12;
  if (!date_start.day)
    date_start.day = 31;
  
  return date_japan >= date_start;
}

bool IsFinishedAiring(const Item& item) {
  if (item.GetAiringStatus(false) == kFinishedAiring)
    return true;

  if (!IsValidDate(item.GetDateEnd()))
    return false;

  if (!IsAiredYet(item))
    return false;

  return GetDateJapan() > item.GetDateEnd();
}

int EstimateLastAiredEpisodeNumber(const Item& item) {
  // Can't estimate for other types of anime
  if (item.GetType() != kTv)
    return 0;

  // TV series air weekly, so the number of weeks that has passed since the day
  // the series started airing gives us the last aired episode. Note that
  // irregularities such as broadcasts being postponed due to sports events make
  // this method unreliable.
  const Date& date_start = item.GetDateStart();
  if (date_start.year && date_start.month && date_start.day) { 
    // To compensate for the fact that we don't know the airing hour,
    // we substract one more day.
    int date_diff = GetDateJapan() - date_start - 1;
    if (date_diff > -1) {
      int number_of_weeks = date_diff / 7;
      if (number_of_weeks < item.GetEpisodeCount()) {
        return number_of_weeks + 1;
      } else {
        return item.GetEpisodeCount();
      }
    }
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

bool CheckEpisodes(Item& item, int number, bool check_folder) {
  // Check folder
  if (check_folder)
    CheckFolder(item);
  if (item.GetFolder().empty()) {
    for (int i = 1; i <= item.GetAvailableEpisodeCount(); i++)
      item.SetEpisodeAvailability(i, false, L"");
    return false;
  }
  
  // Check all episodes
  if (number == -1) {
    SearchFileFolder(item, item.GetFolder(), -1, false);
    return true;

  // Check single episode
  } else {
    if (number == 0) {
      if (item.IsNewEpisodeAvailable())
        return true;
      item.SetNewEpisodePath(L"");
      number = item.GetEpisodeCount() == 1 ? 0 : item.GetMyLastWatchedEpisode() + 1;
    }
    wstring file = SearchFileFolder(item, item.GetFolder(), number, false);
    return !file.empty();
  }
}

bool CheckFolder(Item& item) {
  // Check if current folder still exists
  if (!item.GetFolder().empty() && !FolderExists(item.GetFolder())) {
    LOG(LevelWarning, L"Folder doesn't exist anymore.");
    LOG(LevelWarning, L"Path: " + item.GetFolder());
    item.SetFolder(L"");
  }

  // Search root folders
  if (item.GetFolder().empty()) {
    wstring new_folder;
    foreach_(it, Settings.root_folders) {
      new_folder = SearchFileFolder(item, *it, 0, true);
      if (!new_folder.empty()) {
        item.SetFolder(new_folder);
        Settings.Save();
        break;
      }
    }
  }

  return !item.GetFolder().empty();
}

bool PlayEpisode(Item& item, int number) {
  if (number > item.GetEpisodeCount() && item.GetEpisodeCount() != 0)
    return false;

  wstring file_path;

  SetSharedCursor(IDC_WAIT);

  // Check saved episode path
  if (number == item.GetMyLastWatchedEpisode() + 1)
    if (!item.GetNewEpisodePath().empty())
      if (FileExists(item.GetNewEpisodePath()))
        file_path = item.GetNewEpisodePath();
  
  // Check anime folder
  if (file_path.empty()) {
    CheckFolder(item);
    if (!item.GetFolder().empty()) {
      file_path = SearchFileFolder(item, item.GetFolder(), number, false);
    }
  }

  // Check other folders
  if (file_path.empty()) {
    foreach_(it, Settings.root_folders) {
      file_path = SearchFileFolder(item, *it, number, false);
      if (!file_path.empty())
        break;
    }
  }

  if (file_path.empty()) {
    if (number == 0)
      number = 1;
    MainDialog.ChangeStatus(L"Could not find episode #" + ToWstr(number) +
                            L" (" + item.GetTitle() + L").");
  } else {
    Execute(file_path);
  }

  SetSharedCursor(IDC_ARROW);

  return !file_path.empty();
}

void StartWatching(Item& item, Episode& episode) {
  // Make sure item is in list
  if (!item.IsInList())
    item.AddtoUserList();

  // Change status
  Taiga.play_status = taiga::kPlayStatusPlaying;
  item.SetPlaying(true);

  // Update now playing window
  NowPlayingDialog.SetCurrentId(item.GetId());
  
  // Update anime list window
  int status = item.GetMyRewatching() ? kWatching : item.GetMyStatus();
  if (status != kNotInList) {
    AnimeListDialog.RefreshList(status);
    AnimeListDialog.RefreshTabs(status);
  }
  int list_index = AnimeListDialog.GetListIndex(item.GetId());
  if (list_index > -1) {
    AnimeListDialog.listview.SetItemIcon(list_index, ui::kIcon16_Play);
    AnimeListDialog.listview.RedrawItems(list_index, list_index, true);
    AnimeListDialog.listview.EnsureVisible(list_index);
  }

  // Update main window
  MainDialog.UpdateTip();
  MainDialog.UpdateTitle();
  if (Settings.GetBool(taiga::kSync_Update_GoToNowPlaying))
    MainDialog.navigation.SetCurrentPage(SIDEBAR_ITEM_NOWPLAYING);
  
  // Show balloon tip
  if (Settings.GetBool(taiga::kSync_Notify_Recognized)) {
    Taiga.current_tip_type = taiga::kTipTypeNowPlaying;
    Taskbar.Tip(L"", L"", 0);
    Taskbar.Tip(ReplaceVariables(Settings[taiga::kSync_Notify_Format], episode).c_str(),
                L"Now Playing", NIIF_INFO);
  }
  
  // Check folder
  if (item.GetFolder().empty()) {
    if (episode.folder.empty()) {
      HWND hwnd = MediaPlayers.items[MediaPlayers.index].window_handle;
      episode.folder = MediaPlayers.GetTitleFromProcessHandle(hwnd);
      episode.folder = GetPathOnly(episode.folder);
    }
    if (IsInsideRootFolders(episode.folder)) {
      // Set the folder if only it is under a root folder
      item.SetFolder(episode.folder);
      Settings.Save();
    }
  }

  // Get additional information
  if (item.GetScore().empty() || item.GetSynopsis().empty())
    sync::GetMetadataById(item.GetId());
  
  // Update list
  if (Settings.GetInt(taiga::kSync_Update_Delay) == 0 &&
      !Settings.GetBool(taiga::kSync_Update_WaitPlayer))
    UpdateList(item, episode);
}

void EndWatching(Item& item, Episode episode) {
  // Change status
  Taiga.play_status = taiga::kPlayStatusStopped;
  item.SetPlaying(false);
  
  // Announce
  episode.anime_id = item.GetId();
  Announcer.Do(taiga::kAnnounceToHttp, &episode);
  Announcer.Clear(taiga::kAnnounceToMessenger | taiga::kAnnounceToSkype);

  // Update now playing window
  NowPlayingDialog.SetCurrentId(anime::ID_UNKNOWN);
  
  // Update main window
  episode.anime_id = anime::ID_UNKNOWN;
  MainDialog.UpdateTip();
  MainDialog.UpdateTitle();
  int list_index = AnimeListDialog.GetListIndex(item.GetId());
  if (list_index > -1) {
    AnimeListDialog.listview.SetItemIcon(list_index, StatusToIcon(item.GetAiringStatus()));
    AnimeListDialog.listview.RedrawItems(list_index, list_index, true);
  }
}

bool IsUpdateAllowed(Item& item, const Episode& episode, bool ignore_update_time) {
  if (episode.processed)
    return false;

  if (!ignore_update_time)
    if (Settings.GetInt(taiga::kSync_Update_Delay) > Taiga.ticker_media)
      if (Taiga.ticker_media > -1)
        return false;

  if (item.GetMyStatus() == kCompleted && item.GetMyRewatching() == 0)
    return false;

  int number = GetEpisodeHigh(episode.number);
  int number_low = GetEpisodeLow(episode.number);
  int last_watched = item.GetMyLastWatchedEpisode();

  if (Settings.GetBool(taiga::kSync_Update_OutOfRange))
    if (number_low > last_watched + 1 || number < last_watched + 1)
      return false;

  if (!IsValidEpisode(number, last_watched, item.GetEpisodeCount()))
    return false;

  return true;
}

void UpdateList(Item& item, Episode& episode) {
  if (!IsUpdateAllowed(item, episode, false))
    return;

  episode.processed = true;

  if (Settings.GetBool(taiga::kSync_Update_AskToConfirm)) {
    ConfirmationQueue.Add(episode);
    ConfirmationQueue.Process();
  } else {
    AddToQueue(item, episode, true);
  }
}

void AddToQueue(Item& item, const Episode& episode, bool change_status) {
  // Create event item
  EventItem event_item;
  event_item.anime_id = item.GetId();

  // Set episode number
  event_item.episode = GetEpisodeHigh(episode.number);

  // Set start/finish date
  if (*event_item.episode == 1 && !IsValidDate(item.GetMyDateStart()))
    event_item.date_start = GetDate();
  if (*event_item.episode == item.GetEpisodeCount() && !IsValidDate(item.GetMyDateEnd()))
    event_item.date_finish = GetDate();

  // Set update mode
  if (item.GetMyStatus() == kNotInList) {
    event_item.mode = taiga::kHttpServiceAddLibraryEntry;
  } else {
    event_item.mode = taiga::kHttpServiceUpdateLibraryEntry;
  }

  if (change_status) {
    // Move to completed
    if (item.GetEpisodeCount() == *event_item.episode) {
      event_item.status = kCompleted;
      if (item.GetMyRewatching()) {
        event_item.enable_rewatching = FALSE;
        //event_item.times_rewatched++; // TODO: Enable when MAL adds to API
      }
    // Move to watching
    } else if (item.GetMyStatus() != kWatching || *event_item.episode == 1) {
      if (!item.GetMyRewatching()) {
        event_item.status = kWatching;
      }
    }
  }

  // Add event to queue
  History.queue.Add(event_item);
}

////////////////////////////////////////////////////////////////////////////////

bool GetFansubFilter(int anime_id, vector<wstring>& groups) {
  bool found = false;
  
  foreach_(i, Aggregator.filter_manager.filters) {
    if (found) break;
    foreach_(j, i->anime_ids) {
      if (*j != anime_id) continue;
      if (found) break;
      foreach_(k, i->conditions) {
        if (k->element == FEED_FILTER_ELEMENT_EPISODE_GROUP) {
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
        if (k->element == FEED_FILTER_ELEMENT_EPISODE_GROUP) {
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
    FEED_FILTER_ACTION_PREFER, FEED_FILTER_MATCH_ALL, true,
    L"[Fansub] " + anime_item->GetTitle());
  Aggregator.filter_manager.filters.back().AddCondition(
    FEED_FILTER_ELEMENT_EPISODE_GROUP, FEED_FILTER_OPERATOR_EQUALS,
    group_name);
  Aggregator.filter_manager.filters.back().anime_ids.push_back(anime_id);
  return true;
}

wstring GetImagePath(int anime_id) {
  wstring path = taiga::GetPath(taiga::kPathDatabaseImage);
  if (anime_id > 0) path += ToWstr(anime_id) + L".jpg";
  return path;
}

void GetUpcomingTitles(vector<int>& anime_ids) {
  foreach_c_(item, AnimeDatabase.items) {
    const anime::Item& anime_item = item->second;
    
    const Date& date_start = anime_item.GetDateStart();
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
  foreach_c_(root_folder, Settings.root_folders)
    if (StartsWith(path, *root_folder))
      return true;

  return false;
}

////////////////////////////////////////////////////////////////////////////////

bool IsValidEpisode(int episode, int watched, int total) {
  if ((episode < 0) ||
      (episode < watched) ||
      (episode == watched && total != 1) ||
      (episode > total && total != 0))
    return false;

  return true;
}

int EstimateEpisodeCount(const Item& item) {
  // If we already know the number, we don't need to estimate
  if (item.GetEpisodeCount() > 0)
    return item.GetEpisodeCount();

  int number = 0;

  // Estimate using user information
  if (item.IsInList())
    number = max(item.GetMyLastWatchedEpisode(),
                 item.GetAvailableEpisodeCount());

  // Estimate using airing dates of TV series
  if (item.GetType() == kTv) {
    Date date_start = item.GetDateStart();
    if (IsValidDate(date_start)) {
      Date date_end = item.GetDateEnd();
      // Use current date in Japan if ending date is unknown
      if (!IsValidDate(date_end)) date_end = GetDateJapan();
      // Assuming the series is aired weekly
      number = max(number, (date_end - date_start) / 7);
    }
  }

  // Given all TV series aired since 2000, most them have their episodes
  // spanning one or two seasons. Following is a table of top ten values:
  //
  //   Episodes    Seasons    Percent
  //   ------------------------------
  //         12          1      23.6%
  //         13          1      20.2%
  //         26          2      15.4%
  //         24          2       6.4%
  //         25          2       5.0%
  //         52          4       4.4%
  //         51          4       3.1%
  //         11          1       2.6%
  //         50          4       2.3%
  //         39          3       1.4%
  //   ------------------------------
  //   Total:                   84.6%
  //
  // With that in mind, we can normalize our output at several points.
  if (number < 12) return 13;
  if (number < 24) return 26;
  if (number < 50) return 52;

  // This is a series that has aired for more than a year, which means we cannot
  // estimate for how long it is going to continue.
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

wstring TranslateMyStatus(int value, bool add_count) {
  #define ADD_COUNT() (add_count ? L" (" + ToWstr(AnimeDatabase.GetItemCount(value)) + L")" : L"")
  switch (value) {
    case kNotInList: return L"Not in list";
    case kWatching: return L"Currently watching" + ADD_COUNT();
    case kCompleted: return L"Completed" + ADD_COUNT();
    case kOnHold: return L"On hold" + ADD_COUNT();
    case kDropped: return L"Dropped" + ADD_COUNT();
    case kPlanToWatch: return L"Plan to watch" + ADD_COUNT();
    default: return L"";
  }
  #undef ADD_COUNT
}

wstring TranslateNumber(int value, const wstring& default_char) {
  return value > 0 ? ToWstr(value) : default_char;
}

wstring TranslateStatus(int value) {
  switch (value) {
    case kAiring: return L"Currently airing";
    case kFinishedAiring: return L"Finished airing";
    case kNotYetAired: return L"Not yet aired";
    default: return ToWstr(value);
  }
}

wstring TranslateType(int value) {
  switch (value) {
    case kTv: return L"TV";
    case kOva: return L"OVA";
    case kMovie: return L"Movie";
    case kSpecial: return L"Special";
    case kOna: return L"ONA";
    case kMusic: return L"Music";
    default: return L"";
  }
}

////////////////////////////////////////////////////////////////////////////////
// MyAnimeList only

int TranslateMyStatus(const wstring& value) {
  if (IsEqual(value, L"Currently watching")) {
    return kWatching;
  } else if (IsEqual(value, L"Completed")) {
    return kCompleted;
  } else if (IsEqual(value, L"On hold")) {
    return kOnHold;
  } else if (IsEqual(value, L"Dropped")) {
    return kDropped;
  } else if (IsEqual(value, L"Plan to watch")) {
    return kPlanToWatch;
  } else {
    return 0;
  }
}

int TranslateStatus(const wstring& value) {
  if (IsEqual(value, L"Currently airing")) {
    return kAiring;
  } else if (IsEqual(value, L"Finished airing")) {
    return kFinishedAiring;
  } else if (IsEqual(value, L"Not yet aired")) {
    return kNotYetAired;
  } else {
    return 0;
  }
}

int TranslateType(const wstring& value) {
  if (IsEqual(value, L"TV")) {
    return kTv;
  } else if (IsEqual(value, L"OVA")) {
    return kOva;
  } else if (IsEqual(value, L"Movie")) {
    return kMovie;
  } else if (IsEqual(value, L"Special")) {
    return kSpecial;
  } else if (IsEqual(value, L"ONA")) {
    return kOna;
  } else if (IsEqual(value, L"Music")) {
    return kMusic;
  } else {
    return 0;
  }
}

}  // namespace anime