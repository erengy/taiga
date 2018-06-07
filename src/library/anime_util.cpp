/*
** Taiga
** Copyright (C) 2010-2018, Eren Okka
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

#include <algorithm>

#include "base/file.h"
#include "base/foreach.h"
#include "base/format.h"
#include "base/log.h"
#include "base/process.h"
#include "base/string.h"
#include "library/anime.h"
#include "library/anime_db.h"
#include "library/anime_episode.h"
#include "library/anime_util.h"
#include "library/history.h"
#include "sync/anilist_util.h"
#include "sync/kitsu_util.h"
#include "sync/myanimelist_util.h"
#include "sync/sync.h"
#include "taiga/announce.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "taiga/timer.h"
#include "track/feed.h"
#include "track/media.h"
#include "track/recognition.h"
#include "track/search.h"
#include "ui/ui.h"

namespace anime {

bool IsValidId(int anime_id) {
  return anime_id > ID_UNKNOWN;
}

bool ListHasMissingIds() {
  for (const auto& pair : AnimeDatabase.items) {
    const auto& item = pair.second;
    if (item.GetMyStatus(false) != kNotInList && item.GetMyId().empty()) {
      return true;
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

SeriesStatus GetAiringStatus(const Item& item) {
  auto assume_worst_case = [](Date date) {
    if (!date.month()) date.set_month(12);
    if (!date.day()) date.set_day(31);
    return date;
  };

  const Date now = GetDateJapan();

  if (!IsValidDate(item.GetDateStart()))
    return kNotYetAired;
  const Date start = assume_worst_case(item.GetDateStart());
  if (now < start)
    return kNotYetAired;

  // We don't need to check the end date for single-episode anime
  if (item.GetEpisodeCount() == 1)
    return kFinishedAiring;

  if (!IsValidDate(item.GetDateEnd()))
    return kAiring;
  const Date end = assume_worst_case(item.GetDateEnd());
  if (now <= end)
    return kAiring;

  return kFinishedAiring;
}

bool IsAiredYet(const Item& item) {
  switch (item.GetAiringStatus(false)) {
    case kFinishedAiring:
    case kAiring:
      return true;
  }

  switch (GetAiringStatus(item)) {
    case kFinishedAiring:
    case kAiring:
      return true;
  }

  return false;
}

bool IsFinishedAiring(const Item& item) {
  if (item.GetAiringStatus(false) == kFinishedAiring)
    return true;

  if (GetAiringStatus(item) == kFinishedAiring)
    return true;

  return false;
}

int EstimateDuration(const Item& item) {
  int duration = item.GetEpisodeLength();

  if (duration <= 0) {
    // Approximate duration in minutes
    switch (item.GetType()) {
      default:
      case anime::kTv:      duration = 24; break;
      case anime::kOva:     duration = 24; break;
      case anime::kMovie:   duration = 90; break;
      case anime::kSpecial: duration = 12; break;
      case anime::kOna:     duration = 24; break;
      case anime::kMusic:   duration =  5; break;
    }
  }

  return duration;
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
  if (date_start.year() && date_start.month() && date_start.day()) {
    // To compensate for the fact that we don't know the airing hour,
    // we substract one more day.
    int date_diff = GetDateJapan() - date_start - 1;
    if (date_diff > -1) {
      const int episode_count = item.GetEpisodeCount();
      const int number_of_weeks = date_diff / 7;
      if (!IsValidEpisodeCount(episode_count) ||
          number_of_weeks < episode_count) {
        if (number_of_weeks < 13) {  // Not reliable for longer series
          return number_of_weeks + 1;
        }
      } else {
        return episode_count;
      }
    }
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

// An item's series information will only be updated only if its last modified
// value is significantly older than the new one's. This helps us lower
// the number of requests we send to a service.

bool IsItemOldEnough(const Item& item) {
  if (!item.GetLastModified())
    return true;

  const auto duration = Duration(time(nullptr) - item.GetLastModified());

  if (item.GetAiringStatus() == kFinishedAiring) {
    return duration.days() >= 7;  // 1 week
  } else {
    return duration.hours() >= 1;
  }
}

bool MetadataNeedsRefresh(const Item& item) {
  if (IsItemOldEnough(item))
    return true;

  if (item.GetSynopsis().empty())
    return true;
  if (item.GetGenres().empty())
    return true;
  if (item.GetScore() == kUnknownScore && IsAiredYet(item))
    return true;

  return false;
}

////////////////////////////////////////////////////////////////////////////////

bool IsNsfw(const Item& item) {
  if (item.GetAgeRating() == anime::kAgeRatingR18)
    return true;

  if (item.GetAgeRating() == anime::kUnknownAgeRating) {
    auto& genres = item.GetGenres();
    if (std::find(genres.begin(), genres.end(), L"Hentai") != genres.end())
      return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

bool PlayEpisode(int anime_id, int number) {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (!anime_item)
    return false;

  if (number > anime_item->GetEpisodeCount() &&
      IsValidEpisodeCount(anime_item->GetEpisodeCount()))
    return false;

  if (number == 0)
    number = 1;

  std::wstring file_path;

  // Check saved episode path
  if (number == anime_item->GetMyLastWatchedEpisode() + 1) {
    const std::wstring& next_episode_path = anime_item->GetNextEpisodePath();
    if (!next_episode_path.empty()) {
      if (FileExists(next_episode_path)) {
        file_path = next_episode_path;
      } else {
        LOGD(L"File doesn't exist anymore.\nPath: {}", next_episode_path);
        anime_item->SetEpisodeAvailability(number, false, L"");
      }
    }
  }

  // Scan available episodes
  if (file_path.empty()) {
    ScanAvailableEpisodes(false, anime_item->GetId(), number);
    if (anime_item->IsEpisodeAvailable(number)) {
      file_path = file_search_helper.path_found();
    }
  }

  if (file_path.empty()) {
    ui::ChangeStatusText(L"Could not find episode #{} ({})."_format(
                         number, GetPreferredTitle(*anime_item)));
  } else {
    Execute(file_path);
  }

  return !file_path.empty();
}

bool PlayLastEpisode(int anime_id) {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (!anime_item)
    return false;

  return PlayEpisode(anime_id, anime_item->GetMyLastWatchedEpisode());
}

bool PlayNextEpisode(int anime_id) {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (!anime_item)
    return false;

  int number = anime_item->GetMyLastWatchedEpisode() + 1;

  if (IsValidEpisodeCount(anime_item->GetEpisodeCount()))
    if (number > anime_item->GetEpisodeCount())
      number = 1;  // Play the first episode of completed series

  return PlayEpisode(anime_id, number);
}

bool PlayNextEpisodeOfLastWatchedAnime() {
  int anime_id = ID_UNKNOWN;

  auto get_id_from_history_items = [](const std::vector<HistoryItem>& items) {
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
      const auto& item = *it;
      if (item.episode) {
        auto anime_item = AnimeDatabase.FindItem(item.anime_id);
        if (anime_item && anime_item->GetMyStatus() != anime::kCompleted)
          return item.anime_id;
      }
    }
    return static_cast<int>(anime::ID_UNKNOWN);
  };

  if (!anime::IsValidId(anime_id))
    anime_id = get_id_from_history_items(History.queue.items);
  if (!anime::IsValidId(anime_id))
    anime_id = get_id_from_history_items(History.items);

  return PlayNextEpisode(anime_id);
}

bool PlayRandomAnime() {
  static time_t time_last_checked = 0;
  time_t time_now = time(nullptr);
  if (time_now > time_last_checked + (60 * 2)) {  // 2 minutes
    ScanAvailableEpisodesQuick();
    time_last_checked = time_now;
  }

  std::vector<int> valid_ids;

  for (auto& pair : AnimeDatabase.items) {
    anime::Item& anime_item = pair.second;
    if (!anime_item.IsInList())
      continue;
    if (!anime_item.IsNextEpisodeAvailable())
      continue;
    switch (anime_item.GetMyStatus()) {
      case anime::kNotInList:
      case anime::kCompleted:
      case anime::kDropped:
        continue;
    }
    valid_ids.push_back(anime_item.GetId());
  }

  size_t max_value = valid_ids.size();

  srand(static_cast<unsigned int>(GetTickCount()));

  for (const auto& unused : valid_ids) {
    size_t index = rand() % max_value;
    int anime_id = valid_ids.at(index);
    if (PlayNextEpisode(anime_id))
      return true;
  }

  ui::OnAnimeEpisodeNotFound();
  return false;
}

bool PlayRandomEpisode(int anime_id) {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (!anime_item)
    return false;

  const int total = anime_item->GetMyStatus() == kCompleted ?
      anime_item->GetEpisodeCount() : anime_item->GetMyLastWatchedEpisode() + 1;
  const int max_tries = anime_item->GetFolder().empty() ? 3 : 10;

  srand(static_cast<unsigned int>(GetTickCount()));

  for (int i = 0; i < std::min(total, max_tries); i++) {
    int episode_number = rand() % total + 1;
    if (PlayEpisode(anime_item->GetId(), episode_number))
      return true;
  }

  ui::OnAnimeEpisodeNotFound();
  return false;
}

bool LinkEpisodeToAnime(Episode& episode, int anime_id) {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (!anime_item)
    return false;

  episode.anime_id = anime_id;
  anime_item->AddtoUserList();

  if (episode.elements().empty(anitomy::kElementEpisodeNumber))
    episode.set_episode_number(1);

  auto synonyms = anime_item->GetUserSynonyms();
  synonyms.push_back(CurrentEpisode.anime_title());
  anime_item->SetUserSynonyms(synonyms);
  Meow.UpdateTitles(*anime_item);
  Settings.Save();

  StartWatching(*anime_item, episode);
  ui::ClearStatusText();

  return true;
}

void StartWatching(Item& item, Episode& episode) {
  // Make sure item is in list
  if (!item.IsInList())
    item.AddtoUserList();

  // Change status
  MediaPlayers.play_status = track::recognition::PlayStatus::Playing;
  item.SetPlaying(true);

  ui::OnAnimeWatchingStart(item, episode);

  // Check folder
  if (item.GetFolder().empty()) {
    if (IsInsideLibraryFolders(episode.folder)) {
      // Set the folder if only it is under a library folder
      item.SetFolder(episode.folder);
      Settings.Save();
    }
  }

  // Get additional information
  if (item.GetScore() == kUnknownScore || item.GetSynopsis().empty())
    sync::GetMetadataById(item.GetId());

  // Update list
  if (Settings.GetInt(taiga::kSync_Update_Delay) == 0 &&
      !Settings.GetBool(taiga::kSync_Update_WaitPlayer))
    UpdateList(item, episode);
}

void EndWatching(Item& item, Episode episode) {
  // Change status
  MediaPlayers.play_status = track::recognition::PlayStatus::Stopped;
  item.SetPlaying(false);

  // Announce
  episode.anime_id = item.GetId();
  Announcer.Do(taiga::kAnnounceToHttp, &episode);
  Announcer.Clear(taiga::kAnnounceToDiscord | taiga::kAnnounceToSkype);

  episode.anime_id = anime::ID_UNKNOWN;

  ui::OnAnimeWatchingEnd(item, episode);
}

////////////////////////////////////////////////////////////////////////////////

bool IsDeletedFromList(const Item& item) {
  for (const auto& history_item : History.queue.items)
    if (history_item.anime_id == item.GetId())
      if (history_item.mode == taiga::kHttpServiceDeleteLibraryEntry)
        return true;

  return false;
}

bool IsUpdateAllowed(Item& item, const Episode& episode, bool ignore_update_time) {
  if (episode.processed)
    return false;

  if (!ignore_update_time) {
    auto delay = Settings.GetInt(taiga::kSync_Update_Delay);
    auto ticks = taiga::timers.timer(taiga::kTimerMedia)->ticks();
    if (delay > 0 && ticks > 0)
      return false;
  }

  if (item.GetMyStatus() == kCompleted && item.GetMyRewatching() == 0)
    return false;

  int number = GetEpisodeHigh(episode);
  int number_low = GetEpisodeLow(episode);
  int last_watched = item.GetMyLastWatchedEpisode();

  if (Settings.GetBool(taiga::kSync_Update_OutOfRange))
    if (number_low > last_watched + 1 || number < last_watched + 1)
      return false;

  if (!IsValidEpisodeNumber(number, item.GetEpisodeCount(), last_watched))
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
  history_item.episode = GetEpisodeHigh(episode);

  // Set start/finish date
  if (*history_item.episode == 1 && !IsValidDate(item.GetMyDateStart()))
    history_item.date_start = GetDate();
  if (*history_item.episode == item.GetEpisodeCount() && !IsValidDate(item.GetMyDateEnd()))
    history_item.date_finish = GetDate();

  // Set update mode
  if (item.GetMyStatus() == kNotInList) {
    history_item.mode = taiga::kHttpServiceAddLibraryEntry;
    change_status = true;
  } else {
    history_item.mode = taiga::kHttpServiceUpdateLibraryEntry;
  }

  // Set rewatching status
  if (item.GetMyRewatching()) {
    if (*history_item.episode == item.GetEpisodeCount() && *history_item.episode > 0) {
      history_item.enable_rewatching = FALSE;
      history_item.rewatched_times = item.GetMyRewatchedTimes() + 1;
    }
  }

  if (change_status) {
    // Move to completed
    if (item.GetEpisodeCount() == *history_item.episode && *history_item.episode > 0) {
      history_item.status = kCompleted;
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

void SetMyLastUpdateToNow(Item& item) {
  item.SetMyLastUpdated(ToWstr(time(nullptr)));
}

////////////////////////////////////////////////////////////////////////////////

bool GetFansubFilter(int anime_id, std::vector<std::wstring>& groups) {
  bool found = false;

  for (const auto& filter : Aggregator.filter_manager.filters) {
    if (std::find(filter.anime_ids.begin(), filter.anime_ids.end(),
                  anime_id) != filter.anime_ids.end()) {
      for (const auto& condition : filter.conditions) {
        if (condition.element == kFeedFilterElement_Episode_Group) {
          groups.push_back(condition.value);
          found = true;
        }
      }
    }
  }

  return found;
}

bool SetFansubFilter(int anime_id, const std::wstring& group_name,
                     const std::wstring& video_resolution) {
  const auto find_condition = [](FeedFilter& filter,
                                 FeedFilterElement element) {
    return std::find_if(
        filter.conditions.begin(), filter.conditions.end(),
        [&element](const FeedFilterCondition& condition) {
          return condition.element == element;
        });
  };

  // Check existing filters
  foreach_(filter, Aggregator.filter_manager.filters) {
    auto id = std::find(
        filter->anime_ids.begin(), filter->anime_ids.end(), anime_id);
    if (id == filter->anime_ids.end())
      continue;

    const auto group_condition =
        find_condition(*filter, kFeedFilterElement_Episode_Group);
    if (group_condition == filter->conditions.end())
      continue;

    const auto resolution_condition =
        find_condition(*filter, kFeedFilterElement_Episode_VideoResolution);

    if (filter->anime_ids.size() > 1) {
      filter->anime_ids.erase(id);
      if (group_name.empty()) {
        return true;
      } else {
        break;
      }
    } else {
      if (group_name.empty()) {
        Aggregator.filter_manager.filters.erase(filter);
      } else {
        group_condition->value = group_name;
        if (!video_resolution.empty()) {
          if (resolution_condition != filter->conditions.end()) {
            resolution_condition->value = video_resolution;
          } else {
            filter->AddCondition(kFeedFilterElement_Episode_VideoResolution,
                                 kFeedFilterOperator_Equals, video_resolution);
          }
        }
      }
      return true;
    }
  }

  if (group_name.empty())
    return false;

  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (!anime_item)
    return false;

  // Create new filter
  Aggregator.filter_manager.AddFilter(
      kFeedFilterActionPrefer, kFeedFilterMatchAll, kFeedFilterOptionDefault,
      true, L"[Fansub] " + GetPreferredTitle(*anime_item));
  Aggregator.filter_manager.filters.back().AddCondition(
      kFeedFilterElement_Episode_Group, kFeedFilterOperator_Equals,
      group_name);
  if (!video_resolution.empty()) {
    Aggregator.filter_manager.filters.back().AddCondition(
        kFeedFilterElement_Episode_VideoResolution, kFeedFilterOperator_Equals,
        video_resolution);
  }
  Aggregator.filter_manager.filters.back().anime_ids.push_back(anime_id);
  return true;
}

std::wstring GetImagePath(int anime_id) {
  std::wstring path = taiga::GetPath(taiga::Path::DatabaseImage);
  if (anime_id > 0) path += ToWstr(anime_id) + L".jpg";
  return path;
}

void GetUpcomingTitles(std::vector<int>& anime_ids) {
  for (const auto& pair : AnimeDatabase.items) {
    const anime::Item& anime_item = pair.second;

    const Date& date_start = anime_item.GetDateStart();
    const Date& date_now = GetDateJapan();

    if (!date_start.year() || !date_start.month() || !date_start.day())
      continue;

    if (date_start > date_now &&
        ToDayCount(date_start) < ToDayCount(date_now) + 7) {  // Same week
      anime_ids.push_back(anime_item.GetId());
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

bool IsInsideLibraryFolders(const std::wstring& path) {
  for (auto library_folder : Settings.library_folders) {
    library_folder = GetNormalizedPath(GetFinalPath(library_folder));
    if (StartsWith(path, library_folder))
      return true;
  }

  return false;
}

bool ValidateFolder(Item& item) {
  if (item.GetFolder().empty())
    return false;

  if (FolderExists(item.GetFolder()))
    return true;

  LOGD(L"Folder doesn't exist anymore.\nPath: {}", item.GetFolder());

  item.SetFolder(L"");

  for (int i = 1; i <= item.GetAvailableEpisodeCount(); i++)
    item.SetEpisodeAvailability(i, false, L"");

  return false;
}

////////////////////////////////////////////////////////////////////////////////

int GetEpisodeHigh(const Episode& episode) {
  return episode.episode_number_range().second;
}

int GetEpisodeLow(const Episode& episode) {
  return episode.episode_number_range().first;
}

static std::wstring GetElementRange(anitomy::ElementCategory category,
                                    const Episode& episode) {
  const auto element_count = episode.elements().count(category);

  if (element_count > 1) {
    const auto range = episode.GetElementAsRange(category);
    if (range.second > range.first)
      return ToWstr(range.first) + L"-" + ToWstr(range.second);
  }

  if (element_count > 0)
    return ToWstr(episode.GetElementAsInt(category));

  return std::wstring();
}

std::wstring GetEpisodeRange(const Episode& episode) {
  if (IsEpisodeRange(episode))
    return ToWstr(GetEpisodeLow(episode)) + L"-" +
           ToWstr(GetEpisodeHigh(episode));

  if (!episode.elements().empty(anitomy::kElementEpisodeNumber))
    return ToWstr(episode.episode_number());

  return std::wstring();
}

std::wstring GetVolumeRange(const Episode& episode) {
  return GetElementRange(anitomy::kElementVolumeNumber, episode);
}

std::wstring GetEpisodeRange(const number_range_t& range) {
  if (range.second > range.first)
    return ToWstr(range.first) + L"-" + ToWstr(range.second);

  return ToWstr(range.first);
}

bool IsAllEpisodesAvailable(const Item& item) {
  if (!IsValidEpisodeCount(item.GetEpisodeCount()))
    return false;

  int available_episode_count = item.GetAvailableEpisodeCount();
  bool all_episodes_available = available_episode_count > 0;

  for (int i = 1; i <= available_episode_count; i++) {
    if (!item.IsEpisodeAvailable(i)) {
      all_episodes_available = false;
      break;
    }
  }

  return all_episodes_available;
}

bool IsEpisodeRange(const Episode& episode) {
  if (episode.elements().count(anitomy::kElementEpisodeNumber) < 2)
    return false;
  return GetEpisodeHigh(episode) > GetEpisodeLow(episode);
}

bool IsValidEpisodeCount(int number) {
  return number > 0 && number < 1900;
}

bool IsValidEpisodeNumber(int number, int total) {
  if ((number < 0) ||
      (number > total && IsValidEpisodeCount(total)))
    return false;

  return true;
}

bool IsValidEpisodeNumber(int number, int total, int watched) {
  if (!IsValidEpisodeNumber(number, total) ||
      (number < watched) ||
      (number == watched && total != 1))
    return false;

  return true;
}

std::wstring JoinEpisodeNumbers(const std::vector<int>& input) {
  std::wstring output;

  for (const auto& number : input) {
    if (!output.empty())
      output += L"-";
    output += ToWstr(number);
  }

  return output;
}

void SplitEpisodeNumbers(const std::wstring& input, std::vector<int>& output) {
  if (input.empty())
    return;

  std::vector<std::wstring> numbers;
  Split(input, L"-", numbers);

  for (const auto& number : numbers)
    output.push_back(ToInt(number));
}

int GetLastEpisodeNumber(const Item& item) {
  if (item.GetAiringStatus() == kFinishedAiring)
    return item.GetEpisodeCount();

  int number = 0;

  // Estimate using user information
  number = std::max(number, item.GetMyLastWatchedEpisode());
  if (item.GetAvailableEpisodeCount() != item.GetEpisodeCount())
    number = std::max(number, item.GetAvailableEpisodeCount());

  // Estimate using local information
  number = std::max(number, item.GetLastAiredEpisodeNumber());

  // Estimate using airing dates of TV series
  number = std::max(number, EstimateLastAiredEpisodeNumber(item));

  return number;
}

int EstimateEpisodeCount(const Item& item) {
  // If we already know the number, we don't need to estimate
  if (item.GetEpisodeCount() > 0)
    return item.GetEpisodeCount();

  const int number = GetLastEpisodeNumber(item);

  // Given all TV series aired in 2006-2016, most of them have their episodes
  // spanning one or two seasons. Following is a table of top ten values:
  //
  //   Episodes    Seasons    Percent
  //   ------------------------------
  //         12          1      34.2%
  //         13          1      18.5%
  //         26          2       9.5%
  //         25          2       5.5%
  //         24          2       5.4%
  //         52          4       2.9%
  //         11          1       2.7%
  //         10          1       2.6%
  //         51          4       2.4%
  //         39          3       1.4%
  //   ------------------------------
  //   Total:                   85.1%
  //
  // With that in mind, we can normalize our output at several points.
  if (number < 12) return 12;
  if (number < 24) return 26;
  if (number < 50) return 52;

  // This is a series that has aired for more than a year, which means we cannot
  // estimate for how long it is going to continue.
  return 0;
}

void ChangeEpisode(int anime_id, int value) {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (!anime_item)
    return;

  if (!IsValidEpisodeNumber(value, anime_item->GetEpisodeCount()))
    return;

  Episode episode;
  episode.set_episode_number(value);

  // Allow changing the status to Completed
  bool change_status = value == anime_item->GetEpisodeCount() && value > 0;

  AddToQueue(*anime_item, episode, change_status);
}

void DecrementEpisode(int anime_id) {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (!anime_item)
    return;

  int watched = anime_item->GetMyLastWatchedEpisode();
  auto history_item = History.queue.FindItem(anime_item->GetId(),
                                             QueueSearch::Episode);

  if (history_item && *history_item->episode == watched &&
      watched > anime_item->GetMyLastWatchedEpisode(false)) {
    history_item->enabled = false;
    History.queue.RemoveDisabled();
  } else {
    ChangeEpisode(anime_id, watched - 1);
  }
}

void IncrementEpisode(int anime_id) {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (!anime_item)
    return;

  int watched = anime_item->GetMyLastWatchedEpisode();

  ChangeEpisode(anime_id, watched + 1);
}

////////////////////////////////////////////////////////////////////////////////

const std::wstring& GetPreferredTitle(const Item& item) {
  if (Settings.GetBool(taiga::kApp_List_DisplayEnglishTitles)) {
    return item.GetEnglishTitle(true);
  }
  return item.GetTitle();
}

void GetAllTitles(int anime_id, std::vector<std::wstring>& titles) {
  const auto& anime_item = *AnimeDatabase.FindItem(anime_id);

  auto insert_title = [&titles](const std::wstring& title) {
    if (!title.empty())
      titles.push_back(title);
  };

  insert_title(anime_item.GetTitle());
  insert_title(anime_item.GetEnglishTitle());
  insert_title(anime_item.GetJapaneseTitle());

  for (const auto& synonym : anime_item.GetSynonyms())
    insert_title(synonym);
  for (const auto& synonym : anime_item.GetUserSynonyms())
    insert_title(synonym);
}

// TODO: We may get rid of this function once MAL fixes their API
int GetMyRewatchedTimes(const Item& item) {
  const int rewatched_times = item.GetMyRewatchedTimes();

  if (item.GetMyRewatching()) {
    return std::max(rewatched_times, 1);  // because MAL doesn't tell us the actual value
  } else {
    return rewatched_times;
  }
}

void GetProgressRatios(const Item& item, float& ratio_aired, float& ratio_watched) {
  ratio_aired = 0.0f;
  ratio_watched = 0.0f;

  const int eps_total = EstimateEpisodeCount(item);
  const int eps_aired = GetLastEpisodeNumber(item);
  const int eps_watched = item.GetMyLastWatchedEpisode(true);

  // Episode count is known or estimated
  if (eps_total) {
    if (eps_aired)
      ratio_aired = eps_aired / static_cast<float>(eps_total);
    if (eps_watched)
      ratio_watched = eps_watched / static_cast<float>(eps_total);
  // Episode count is unknown
  } else {
    if (eps_aired)
      ratio_aired = eps_aired > eps_watched ? 0.85f : 0.8f;
    if (eps_watched)
      ratio_watched = std::min(0.8f, (0.8f * eps_watched) / eps_aired);
  }

  // Limit values so that they don't exceed total episodes
  ratio_aired = std::min(ratio_aired, 1.0f);
  ratio_watched = std::min(ratio_watched, 1.0f);
}

////////////////////////////////////////////////////////////////////////////////

std::wstring TranslateMyStatus(int value, bool add_count) {
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

std::wstring TranslateNumber(int value, const std::wstring& default_char) {
  return value > 0 ? ToWstr(value) : default_char;
}

std::wstring TranslateMyScore(int value, const std::wstring& default_char) {
  if (!value)
    return default_char;

  switch (taiga::GetCurrentServiceId()) {
    default:
      return ToWstr(value);
    case sync::kMyAnimeList:
      return sync::myanimelist::TranslateMyRating(value, false);
    case sync::kKitsu:
      return sync::kitsu::TranslateMyRating(
          value, sync::kitsu::GetRatingSystem());
    case sync::kAniList:
      return sync::anilist::TranslateMyRating(
          value, sync::anilist::GetRatingSystem());
  }
}

std::wstring TranslateMyDate(const Date& value, const std::wstring& default_char) {
  return IsValidDate(value) ? TranslateDate(value) : default_char;
}

std::wstring TranslateMyScoreFull(int value) {
  switch (taiga::GetCurrentServiceId()) {
    default:
      return ToWstr(value);
    case sync::kMyAnimeList:
      return sync::myanimelist::TranslateMyRating(value, true);
    case sync::kKitsu:
      return sync::kitsu::TranslateMyRating(
          value, sync::kitsu::GetRatingSystem());
    case sync::kAniList:
      return sync::anilist::TranslateMyRating(
          value, sync::anilist::GetRatingSystem());
  }
}

std::wstring TranslateScore(double value) {
  switch (taiga::GetCurrentServiceId()) {
    default:
    case sync::kMyAnimeList:
      return ToWstr(value, 2);
    case sync::kKitsu:
      return ToWstr(sync::kitsu::TranslateSeriesRatingTo(value), 2) + L"%";
    case sync::kAniList:
      return ToWstr(sync::anilist::TranslateSeriesRatingTo(value), 0) + L"%";
  }
}

std::wstring TranslateStatus(int value) {
  switch (value) {
    case kAiring: return L"Currently airing";
    case kFinishedAiring: return L"Finished airing";
    case kNotYetAired: return L"Not yet aired";
    default: return ToWstr(value);
  }
}

std::wstring TranslateType(int value) {
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

int TranslateType(const std::wstring& value) {
  static const std::map<std::wstring, anime::SeriesType> types{
    {L"tv", kTv},
    {L"ova", kOva}, {L"oav", kOva},
    {L"movie", kMovie}, {L"gekijouban", kMovie},
    {L"special", kSpecial},
    {L"ona", kOna},
    {L"music", kMusic},
  };

  auto it = types.find(ToLower_Copy(value));
  return it != types.end() ? it->second : anime::kUnknownType;
}

int TranslateResolution(const std::wstring& str) {
  // *###x###*
  if (str.length() > 6) {
    int pos = InStr(str, L"x", 0);
    if (pos > -1) {
      for (unsigned int i = 0; i < str.length(); i++)
        if (i != pos && !IsNumericChar(str.at(i)))
          return 0;
      return ToInt(str.substr(pos + 1));
    }

  // *###p
  } else if (str.length() > 3) {
    if (str.at(str.length() - 1) == 'p') {
      for (unsigned int i = 0; i < str.length() - 1; i++)
        if (!IsNumericChar(str.at(i)))
          return 0;
      return ToInt(str.substr(0, str.length() - 1));
    }
  }

  // 1080, 720, 480
  if (IsNumericString(str)) {
    const auto height = ToInt(str);
    switch (height) {
      case 1080:
      case 720:
      case 480:
        return height;
    }
  }

  return 0;
}

std::wstring NormalizeResolution(const std::wstring& resolution) {
  const auto height = TranslateResolution(resolution);
  return height ? L"{}p"_format(height) : resolution;
}

}  // namespace anime