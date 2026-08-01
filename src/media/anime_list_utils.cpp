/**
 * Taiga
 * Copyright (C) 2010-2026, Eren Okka
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

#include "anime_list_utils.hpp"

#include <ctime>

#include "media/anime.hpp"
#include "media/anime_db.hpp"
#include "media/anime_history.hpp"
#include "media/anime_list.hpp"

namespace anime::list {

float getProgressRatio(const Details* item, const Entry* entry) {
  const auto progress = (entry ? entry->watched_episodes : 0);
  const auto total = (item ? item->episode_count : 0);
  if (!total) return 0.8f;
  return std::min(progress / static_cast<float>(total), 1.0f);
}

void save(Entry entry) {
  const auto previous = db.entry(entry.anime_id);
  const bool episodeChanged = !previous || previous->watched_episodes != entry.watched_episodes;

  entry.last_updated = std::time(nullptr);
  db.updateEntry(entry);

  if (episodeChanged && entry.watched_episodes > 0) {
    history.add(entry.anime_id, entry.watched_episodes, entry.last_updated);
  }
}

void remove(const int animeId) {
  const auto entry = db.entry(animeId);
  if (!entry) return;

  auto updated = *entry;
  updated.pending_delete = true;
  updated.last_updated = std::time(nullptr);

  db.updateEntry(updated);
}

}  // namespace anime::list
