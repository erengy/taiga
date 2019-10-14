/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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

#include "track/play.h"

#include "base/file.h"
#include "base/format.h"
#include "base/log.h"
#include "base/random.h"
#include "media/anime.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "media/library/queue.h"
#include "taiga/settings.h"
#include "track/scanner.h"
#include "ui/ui.h"

namespace track {

bool PlayEpisode(int anime_id, int number) {
  auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return false;

  if (number > anime_item->GetEpisodeCount() &&
      anime::IsValidEpisodeCount(anime_item->GetEpisodeCount()))
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
      file_path = track::scanner.path_found();
    }
  }

  if (file_path.empty()) {
    ui::ChangeStatusText(L"Could not find episode #{} ({})."_format(
                         number, GetPreferredTitle(*anime_item)));
    return false;
  }

  const auto player_path = taiga::settings.GetLibraryMediaPlayerPath();
  if (player_path.empty()) {
    return Execute(file_path);
  } else {
    return Execute(player_path, LR"("{}")"_format(file_path));
  }
}

bool PlayLastEpisode(int anime_id) {
  auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return false;

  return PlayEpisode(anime_id, anime_item->GetMyLastWatchedEpisode());
}

bool PlayNextEpisode(int anime_id) {
  auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return false;

  int number = anime_item->GetMyLastWatchedEpisode() + 1;

  if (anime::IsValidEpisodeCount(anime_item->GetEpisodeCount()))
    if (number > anime_item->GetEpisodeCount())
      number = 1;  // Play the first episode of completed series

  return PlayEpisode(anime_id, number);
}

bool PlayNextEpisodeOfLastWatchedAnime() {
  int anime_id = anime::ID_UNKNOWN;

  auto get_id_from_history_items = [](const std::vector<library::QueueItem>& items) {
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
      const auto& item = *it;
      if (item.episode) {
        auto anime_item = anime::db.Find(item.anime_id);
        if (anime_item && anime_item->GetMyStatus() != anime::kCompleted)
          return item.anime_id;
      }
    }
    return static_cast<int>(anime::ID_UNKNOWN);
  };

  if (!anime::IsValidId(anime_id))
    anime_id = get_id_from_history_items(library::queue.items);
  // @TODO
  //if (!anime::IsValidId(anime_id))
  //  anime_id = get_id_from_history_items(History.items);

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

  for (const auto& [id, anime_item] : anime::db.items) {
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

  Random::shuffle(valid_ids);

  for (const auto& anime_id : valid_ids) {
    if (PlayNextEpisode(anime_id))
      return true;
  }

  ui::OnAnimeEpisodeNotFound(L"Play Random Episode");
  return false;
}

bool PlayRandomEpisode(int anime_id) {
  auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return false;

  const int total = anime_item->GetMyStatus() == anime::kCompleted ?
      anime_item->GetEpisodeCount() : anime_item->GetMyLastWatchedEpisode() + 1;
  const int max_tries = anime_item->GetFolder().empty() ? 3 : 10;

  for (int i = 0; i < std::min(total, max_tries); i++) {
    const int episode_number = Random::get(1, total);
    if (PlayEpisode(anime_item->GetId(), episode_number))
      return true;
  }

  ui::OnAnimeEpisodeNotFound(L"Play Random Episode");
  return false;
}

}  // namespace track
