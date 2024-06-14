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

#include "media/library/list_util.h"

#include "media/anime_db.h"
#include "media/anime_util.h"
#include "media/library/queue.h"
#include "track/episode.h"

namespace anime {

void ChangeEpisode(int anime_id, int value) {
  const auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return;

  if (!IsValidEpisodeNumber(value, anime_item->GetEpisodeCount()))
    return;

  Episode episode;
  episode.set_episode_number(value);

  // Allow changing the status to Completed
  const bool change_status =
      value == anime_item->GetEpisodeCount() && value > 0;

  AddToQueue(*anime_item, episode, change_status);
}

void DecrementEpisode(int anime_id) {
  const auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return;

  const int watched = anime_item->GetMyLastWatchedEpisode();
  const auto queue_item = library::queue.FindItem(
      anime_item->GetId(), library::QueueSearch::Episode);

  if (queue_item && *queue_item->episode == watched &&
      watched > anime_item->GetMyLastWatchedEpisode(false)) {
    queue_item->enabled = false;
    library::queue.RemoveDisabled();
    return;
  }

  ChangeEpisode(anime_id, watched - 1);
}

void IncrementEpisode(int anime_id) {
  const auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return;

  const int watched = anime_item->GetMyLastWatchedEpisode();

  ChangeEpisode(anime_id, watched + 1);
}

}  // namespace anime
