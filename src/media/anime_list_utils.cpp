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
#include "sync/queue.hpp"

namespace anime::list {

float getProgressRatio(const Details* item, const Entry* entry) {
  const auto progress = (entry ? entry->watched_episodes : 0);
  const auto total = (item ? item->episode_count : 0);
  if (!total) return 0.8f;
  return std::min(progress / static_cast<float>(total), 1.0f);
}

void save(Entry entry) {
  const auto previous = db.entry(entry.anime_id);
  const Entry baseline = previous ? *previous : Entry{.anime_id = entry.anime_id};

  Fields dirty;
  if (baseline.watched_episodes != entry.watched_episodes) dirty |= Field::Episode;
  if (baseline.score != entry.score) dirty |= Field::Score;
  if (baseline.status != entry.status) dirty |= Field::Status;
  if (baseline.rewatching != entry.rewatching) dirty |= Field::Rewatching;
  if (baseline.rewatched_times != entry.rewatched_times) dirty |= Field::RewatchedTimes;
  if (baseline.date_started != entry.date_started) dirty |= Field::DateStarted;
  if (baseline.date_completed != entry.date_completed) dirty |= Field::DateCompleted;
  if (baseline.notes != entry.notes) dirty |= Field::Notes;

  entry.pending_delete = false;
  entry.last_updated = std::time(nullptr);
  db.updateEntry(entry);

  if ((dirty & Field::Episode) && entry.watched_episodes > 0) {
    history.add(entry.anime_id, entry.watched_episodes, entry.last_updated);
  }

  if (dirty) {
    sync::queue.push(entry.anime_id, dirty);
  }
}

void remove(const int animeId) {
  const auto entry = db.entry(animeId);
  if (!entry) return;

  auto updated = *entry;
  updated.pending_delete = true;
  updated.last_updated = std::time(nullptr);

  db.updateEntry(updated);

  sync::queue.pushDelete(animeId);
}

}  // namespace anime::list
