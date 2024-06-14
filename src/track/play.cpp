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

#include "track/play.h"

#include <numeric>

#include "base/file.h"
#include "base/format.h"
#include "base/log.h"
#include "base/random.h"
#include "media/anime.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "media/library/history.h"
#include "media/library/queue.h"
#include "taiga/settings.h"
#include "track/scanner.h"
#include "ui/ui.h"

namespace track {

bool PlayEpisode(const int anime_id, int number) {
  const auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return false;

  if (!anime::IsValidEpisodeNumber(number, anime_item->GetEpisodeCount()))
    return false;

  if (number == 0)
    number = 1;

  std::wstring file_path;

  // Check saved episode path
  if (number == anime_item->GetMyLastWatchedEpisode() + 1) {
    const auto& next_episode_path = anime_item->GetNextEpisodePath();
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
    ScanAvailableEpisodes(false, anime_id, number);
    if (anime_item->IsEpisodeAvailable(number)) {
      file_path = track::scanner.path_found();
    }
  }

  if (file_path.empty()) {
    ui::ChangeStatusText(L"Could not find episode #{} ({})."_format(
        number, anime::GetPreferredTitle(*anime_item)));
    return false;
  }

  const auto player_path = taiga::settings.GetLibraryMediaPlayerPath();
  if (player_path.empty()) {
    return Execute(file_path);
  } else {
    return Execute(player_path, LR"("{}")"_format(file_path));
  }
}

bool PlayLastEpisode(const int anime_id) {
  const auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return false;

  return PlayEpisode(anime_id, anime_item->GetMyLastWatchedEpisode());
}

bool PlayNextEpisode(const int anime_id) {
  const auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return false;

  int number = anime_item->GetMyLastWatchedEpisode() + 1;

  if (!anime::IsValidEpisodeNumber(number, anime_item->GetEpisodeCount()))
    number = 1;  // Play the first episode of completed series

  return PlayEpisode(anime_id, number);
}

bool PlayNextEpisodeOfLastWatchedAnime() {
  const auto check_item = [](const int anime_id) {
    const auto anime_item = anime::db.Find(anime_id);
    return anime_item &&
           anime_item->GetMyStatus() != anime::MyStatus::Completed;
  };

  const auto& queue_items = library::queue.items;
  for (auto it = queue_items.rbegin(); it != queue_items.rend(); ++it) {
    if (it->episode && check_item(it->anime_id)) {
      return PlayNextEpisode(it->anime_id);
    }
  }

  const auto& history_items = library::history.items;
  for (auto it = history_items.rbegin(); it != history_items.rend(); ++it) {
    if (it->episode && check_item(it->anime_id)) {
      return PlayNextEpisode(it->anime_id);
    }
  }

  return false;
}

bool PlayRandomAnime() {
  static time_t time_last_checked = 0;
  time_t time_now = time(nullptr);
  if (time_now > time_last_checked + (60 * 2)) {  // 2 minutes
    ScanAvailableEpisodesQuick();
    time_last_checked = time_now;
  }

  std::vector<int> valid_ids;

  for (const auto& [id, anime_item] : anime::db.items) {
    if (!anime_item.IsInList())
      continue;
    if (!anime_item.IsNextEpisodeAvailable())
      continue;
    switch (anime_item.GetMyStatus()) {
      case anime::MyStatus::NotInList:
      case anime::MyStatus::Completed:
      case anime::MyStatus::Dropped:
        continue;
    }
    valid_ids.push_back(anime_item.GetId());
  }

  Random::shuffle(valid_ids);

  for (const auto& anime_id : valid_ids) {
    if (PlayNextEpisode(anime_id))
      return true;
  }

  ui::OnAnimeEpisodeNotFound(L"Play Random Episode");
  return false;
}

bool PlayRandomEpisode(const int anime_id) {
  const auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return false;

  const int total = anime_item->GetMyStatus() == anime::MyStatus::Completed
                        ? anime_item->GetEpisodeCount()
                        : anime_item->GetMyLastWatchedEpisode() + 1;

  const int max_tries = anime_item->GetFolder().empty() ? 3 : 10;

  std::vector<int> episodes(std::min(total, max_tries));
  std::iota(episodes.begin(), episodes.end(), 1);
  Random::shuffle(episodes);

  for (const auto& episode_number : episodes) {
    if (PlayEpisode(anime_id, episode_number))
      return true;
  }

  ui::OnAnimeEpisodeNotFound(L"Play Random Episode");
  return false;
}

}  // namespace track
