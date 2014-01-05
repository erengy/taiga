/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#include "base/common.h"
#include "base/file.h"
#include "base/foreach.h"
#include "base/logger.h"
#include "base/process.h"
#include "base/string.h"
#include "library/anime.h"
#include "library/anime_db.h"
#include "library/anime_episode.h"
#include "library/anime_util.h"
#include "library/history.h"
#include "sync/sync.h"
#include "taiga/announce.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "track/feed.h"
#include "track/media.h"
#include "ui/ui.h"

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
    ui::ChangeStatusText(L"Could not find episode #" + ToWstr(number) +
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

  ui::OnAnimeWatchingStart(item, episode);

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

  episode.anime_id = anime::ID_UNKNOWN;

  ui::OnAnimeWatchingEnd(item, episode);
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
  // Create history item
  HistoryItem history_item;
  history_item.anime_id = item.GetId();

  // Set episode number
  history_item.episode = GetEpisodeHigh(episode.number);

  // Set start/finish date
  if (*history_item.episode == 1 && !IsValidDate(item.GetMyDateStart()))
    history_item.date_start = GetDate();
  if (*history_item.episode == item.GetEpisodeCount() && !IsValidDate(item.GetMyDateEnd()))
    history_item.date_finish = GetDate();

  // Set update mode
  if (item.GetMyStatus() == kNotInList) {
    history_item.mode = taiga::kHttpServiceAddLibraryEntry;
  } else {
    history_item.mode = taiga::kHttpServiceUpdateLibraryEntry;
  }

  if (change_status) {
    // Move to completed
    if (item.GetEpisodeCount() == *history_item.episode) {
      history_item.status = kCompleted;
      if (item.GetMyRewatching()) {
        history_item.enable_rewatching = FALSE;
        //history_item.times_rewatched++; // TODO: Enable when MAL adds to API
      }
    // Move to watching
    } else if (item.GetMyStatus() != kWatching || *history_item.episode == 1) {
      if (!item.GetMyRewatching()) {
        history_item.status = kWatching;
      }
    }
  }

  // Add to queue
  History.queue.Add(history_item);
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

}  // namespace anime