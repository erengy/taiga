/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>

#include <nstd/algorithm.hpp>

#include "media/anime_util.h"

#include "base/file.h"
#include "base/format.h"
#include "base/html.h"
#include "base/log.h"
#include "base/string.h"
#include "media/anime.h"
#include "media/anime_db.h"
#include "media/library/queue.h"
#include "sync/sync.h"
#include "taiga/announce.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/timer.h"
#include "track/episode.h"
#include "track/episode_util.h"
#include "track/media.h"
#include "track/play.h"
#include "track/recognition.h"
#include "ui/ui.h"

namespace anime {

bool IsValidId(int anime_id) {
  return anime_id > ID_UNKNOWN;
}

////////////////////////////////////////////////////////////////////////////////

SeriesStatus GetAiringStatus(const Item& item) {
  // We can trust the active service when it says something happened in the past
  // (but not the opposite, because the information might be stale).
  // This is the most common case, and it also handles the rare case where an
  // anime has finished airing but the dates are completely unknown.
  if (item.GetAiringStatus(false) == SeriesStatus::FinishedAiring)
    return SeriesStatus::FinishedAiring;

  const auto& date_start = item.GetDateStart();
  const auto& date_end = item.GetDateEnd();

  // It is now safe to assume that an anime with unknown dates has not yet
  // aired, considering we have covered the rare case mentioned above.
  if (date_start.empty() && date_end.empty())
    return SeriesStatus::NotYetAired;

  const Date today = GetDateJapan();

  if (!date_end.empty() && date_end < today)
    return SeriesStatus::FinishedAiring;

  if (!date_start.empty() && date_start <= today)
    return SeriesStatus::Airing;

  return SeriesStatus::NotYetAired;
}

bool IsAiredYet(const Item& item) {
  switch (item.GetAiringStatus(false)) {
    case SeriesStatus::FinishedAiring:
    case SeriesStatus::Airing:
      return true;
  }

  switch (GetAiringStatus(item)) {
    case SeriesStatus::FinishedAiring:
    case SeriesStatus::Airing:
      return true;
  }

  return false;
}

bool IsFinishedAiring(const Item& item) {
  return item.GetAiringStatus() == SeriesStatus::FinishedAiring;
}

int EstimateDuration(const Item& item) {
  int duration = item.GetEpisodeLength();

  if (duration <= 0) {
    // Approximate duration in minutes
    switch (item.GetType()) {
      default:
      case SeriesType::Tv:      duration = 24; break;
      case SeriesType::Ova:     duration = 24; break;
      case SeriesType::Movie:   duration = 90; break;
      case SeriesType::Special: duration = 12; break;
      case SeriesType::Ona:     duration = 24; break;
      case SeriesType::Music:   duration =  5; break;
    }
  }

  return duration;
}

int EstimateLastAiredEpisodeNumber(const Item& item) {
  // Can't estimate for other types of anime
  if (item.GetType() != SeriesType::Tv)
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

  if (item.GetAiringStatus() == SeriesStatus::FinishedAiring) {
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
  if (item.GetAgeRating() == anime::AgeRating::R18)
    return true;

  if (item.GetAgeRating() == anime::AgeRating::Unknown) {
    if (nstd::contains(item.GetGenres(), std::wstring{L"Hentai"}))
      return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

bool StartNewRewatch(int anime_id) {
  auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return false;

  library::QueueItem queue_item;
  queue_item.anime_id = anime_item->GetId();
  queue_item.status = MyStatus::Watching;
  queue_item.enable_rewatching = true;
  queue_item.episode = 0;
  library::queue.Add(queue_item);

  if (track::PlayEpisode(anime_item->GetId(), 0)) {
    return true;
  }

  ui::OnAnimeEpisodeNotFound(L"Start New Rewatch");
  return false;
}

bool LinkEpisodeToAnime(Episode& episode, int anime_id) {
  auto anime_item = anime::db.Find(anime_id);

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
  taiga::settings.Save();

  StartWatching(*anime_item, episode);
  ui::ClearStatusText();

  return true;
}

void StartWatching(Item& item, Episode& episode) {
  // Make sure item is in list
  if (!item.IsInList())
    item.AddtoUserList();

  // Change status
  track::media_players.play_status = track::recognition::PlayStatus::Playing;

  ui::OnAnimeWatchingStart(item, episode);

  // Check folder
  if (item.GetFolder().empty()) {
    if (IsInsideLibraryFolders(episode.folder)) {
      // Set the folder if only it is under a library folder
      item.SetFolder(episode.folder);
      taiga::settings.Save();
    }
  }

  // Get additional information
  if (item.GetScore() == kUnknownScore || item.GetSynopsis().empty())
    sync::GetMetadataById(item.GetId());

  // Update list
  if (taiga::settings.GetSyncUpdateDelay() == 0 &&
      !taiga::settings.GetSyncUpdateWaitPlayer())
    UpdateList(item, episode);
}

void EndWatching(Item& item, Episode episode) {
  // Change status
  track::media_players.play_status = track::recognition::PlayStatus::Stopped;

  // Announce
  episode.anime_id = item.GetId();
  taiga::announcer.Do(taiga::kAnnounceToHttp, &episode);
  taiga::announcer.Clear(taiga::kAnnounceToDiscord);

  episode.anime_id = anime::ID_UNKNOWN;

  ui::OnAnimeWatchingEnd(item, episode);
}

////////////////////////////////////////////////////////////////////////////////

bool IsDeletedFromList(const Item& item) {
  for (const auto& queue_item : library::queue.items)
    if (queue_item.anime_id == item.GetId())
      if (queue_item.mode == library::QueueItemMode::Delete)
        return true;

  return false;
}

bool IsUpdateAllowed(const Item& item, const Episode& episode, bool ignore_update_time) {
  if (episode.processed)
    return false;

  if (!ignore_update_time) {
    const auto delay = taiga::settings.GetSyncUpdateDelay();
    const auto ticks = taiga::timers.timer(taiga::kTimerMedia)->ticks();
    if (delay > 0 && ticks > 0)
      return false;
  }

  if (item.GetMyStatus() == MyStatus::Completed && !item.GetMyRewatching())
    return false;

  int number = GetEpisodeHigh(episode);
  int number_low = GetEpisodeLow(episode);
  int last_watched = item.GetMyLastWatchedEpisode();

  if (taiga::settings.GetSyncUpdateOutOfRange())
    if (number_low > last_watched + 1 || number < last_watched + 1)
      return false;

  if (!IsValidEpisodeNumber(number, item.GetEpisodeCount(), last_watched))
    return false;

  return true;
}

void UpdateList(const Item& item, Episode& episode) {
  if (!IsUpdateAllowed(item, episode, false))
    return;

  episode.processed = true;

  if (taiga::settings.GetSyncUpdateAskToConfirm()) {
    library::confirmation_queue.Add(episode);
    library::confirmation_queue.Process();
  } else {
    AddToQueue(item, episode, true);
  }
}

void AddToQueue(const Item& item, const Episode& episode, bool change_status) {
  // Create history item
  library::QueueItem queue_item;
  queue_item.anime_id = item.GetId();

  // Set episode number
  queue_item.episode = GetEpisodeHigh(episode);

  // Set start/finish date
  if (*queue_item.episode == 1 && !IsValidDate(item.GetMyDateStart()))
    queue_item.date_start = GetDate();
  if (*queue_item.episode == item.GetEpisodeCount() && !IsValidDate(item.GetMyDateEnd()))
    queue_item.date_finish = GetDate();

  // Set update mode
  if (item.GetMyStatus() == MyStatus::NotInList) {
    queue_item.mode = library::QueueItemMode::Add;
    change_status = true;
  } else {
    queue_item.mode = library::QueueItemMode::Update;
  }

  // Set rewatching status
  if (item.GetMyRewatching()) {
    if (*queue_item.episode == item.GetEpisodeCount() && *queue_item.episode > 0) {
      queue_item.enable_rewatching = false;
      queue_item.rewatched_times = item.GetMyRewatchedTimes() + 1;
    }
  }

  if (change_status) {
    // Move to completed
    if (item.GetEpisodeCount() == *queue_item.episode && *queue_item.episode > 0) {
      queue_item.status = MyStatus::Completed;
    // Move to watching
    } else if (item.GetMyStatus() != MyStatus::Watching || *queue_item.episode == 1) {
      if (!item.GetMyRewatching()) {
        queue_item.status = MyStatus::Watching;
      }
    }
  }

  // Add to queue
  library::queue.Add(queue_item);
}

void SetMyLastUpdateToNow(Item& item) {
  item.SetMyLastUpdated(ToWstr(time(nullptr)));
}

////////////////////////////////////////////////////////////////////////////////

std::wstring GetImagePath(int anime_id) {
  std::wstring path = taiga::GetPath(taiga::Path::DatabaseImage);
  if (anime_id > 0) path += ToWstr(anime_id) + L".jpg";
  return path;
}

void GetUpcomingTitles(std::vector<int>& anime_ids) {
  for (const auto& [id, anime_item] : anime::db.items) {
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
  for (auto library_folder : taiga::settings.GetLibraryFolders()) {
    library_folder = GetNormalizedPath(GetFinalPath(library_folder));
    if (StartsWith(path, library_folder))
      return true;
  }

  return false;
}

bool ValidateFolder(Item& item) {
  const auto folder = item.GetFolder();

  if (folder.empty())
    return false;

  if (FolderExists(folder))
    return true;

  LOGD(L"Folder doesn't exist anymore.\nPath: {}", folder);

  item.SetFolder(L"");

  for (int i = 1; i <= item.GetAvailableEpisodeCount(); ++i) {
    item.SetEpisodeAvailability(i, false, L"");
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

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

bool IsValidEpisodeCount(int number) {
  return number > 0 && number < kMaxEpisodeCount;
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

int GetLastEpisodeNumber(const Item& item) {
  if (item.GetAiringStatus() == SeriesStatus::FinishedAiring)
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

////////////////////////////////////////////////////////////////////////////////

const std::wstring& GetPreferredTitle(const Item& item) {
  switch (taiga::settings.GetAppListTitleLanguagePreference()) {
    default:
    case anime::TitleLanguage::Romaji:
      return item.GetTitle();
    case anime::TitleLanguage::English:
      return item.GetEnglishTitle(true);
    case anime::TitleLanguage::Native: {
      const auto& native_title = item.GetJapaneseTitle();
      return !native_title.empty() ? native_title : item.GetTitle();
    }
  }
}

void GetAllTitles(int anime_id, std::vector<std::wstring>& titles) {
  const auto& anime_item = *anime::db.Find(anime_id);

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

std::vector<std::wstring> GetStudiosAndProducers(const Item& item) {
  std::vector<std::wstring> studios = item.GetStudios();
  const std::vector<std::wstring> producers = item.GetProducers();
  studios.insert(studios.end(), producers.begin(), producers.end());
  return studios;
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

bool IsValidDate(const Date& date) {
  return date.year() > 0;
}

////////////////////////////////////////////////////////////////////////////////

std::wstring NormalizeSynopsis(std::wstring str) {
  while (ReplaceString(str, L"\r\n", L"\n"));
  ReplaceString(str, L"<br>", L"\n");
  while (ReplaceString(str, L"\n\n\n", L"\n\n"));
  ReplaceString(str, L"\n", L"\r\n");

  DecodeHtmlEntities(str);
  StripHtmlTags(str);

  return str;
}

}  // namespace anime
