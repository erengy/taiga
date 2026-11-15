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

#include <QDate>
#include <ctime>

#include "media/anime.hpp"
#include "media/anime_db.hpp"
#include "media/anime_history.hpp"
#include "media/anime_list.hpp"
#include "sync/queue.hpp"

namespace anime::list {

float getProgressRatio(const Details* item, const Entry* entry) {
  const auto progress = (entry ? entry->watchedEpisodes : 0);
  const auto total = (item ? item->episodeCount : 0);
  if (!total) return 0.8f;
  return std::min(progress / static_cast<float>(total), 1.0f);
}

bool isInList(const Entry* entry) {
  // Pending removal counts as not being in list.
  return entry && entry->status != Status::NotInList && !entry->pendingDelete;
}

Entry entryWithEpisodeWatched(const Details& item, const Entry* entry, const int number) {
  auto updated = isInList(entry) ? *entry : Entry{.animeId = item.id};

  const bool isFinalEpisode = item.episodeCount > 0 && number == item.episodeCount;
  const FuzzyDate today{QDate::currentDate().toStdSysDays()};

  updated.watchedEpisodes = number;

  if (number == 1 && !updated.dateStarted) updated.dateStarted = today;
  if (isFinalEpisode && !updated.dateCompleted) updated.dateCompleted = today;

  if (updated.rewatching && isFinalEpisode) {
    updated.rewatching = false;
    updated.rewatchedTimes++;
  }

  if (isFinalEpisode) {
    updated.status = Status::Completed;
  } else if (!updated.rewatching) {
    updated.status = Status::Watching;
  }

  return updated;
}

void save(Entry entry) {
  const auto previous = db()->entry(entry.animeId);
  const Entry baseline = previous ? *previous : Entry{.animeId = entry.animeId};

  Fields dirty;
  if (baseline.watchedEpisodes != entry.watchedEpisodes) dirty |= Field::Episode;
  if (baseline.score != entry.score) dirty |= Field::Score;
  if (baseline.status != entry.status) dirty |= Field::Status;
  if (baseline.rewatching != entry.rewatching) dirty |= Field::Rewatching;
  if (baseline.rewatchedTimes != entry.rewatchedTimes) dirty |= Field::RewatchedTimes;
  if (baseline.dateStarted != entry.dateStarted) dirty |= Field::DateStarted;
  if (baseline.dateCompleted != entry.dateCompleted) dirty |= Field::DateCompleted;
  if (baseline.notes != entry.notes) dirty |= Field::Notes;

  if (!dirty) return;

  entry.pendingDelete = false;
  entry.lastUpdated = std::time(nullptr);
  db()->updateEntry(entry);

  if ((dirty & Field::Episode) && entry.watchedEpisodes > 0) {
    history()->add(entry.animeId, entry.watchedEpisodes, entry.lastUpdated);
  }

  sync::queue()->push(entry.animeId, dirty);
}

void remove(const int animeId) {
  const auto entry = db()->entry(animeId);
  if (!entry) return;

  auto updated = *entry;
  updated.pendingDelete = true;
  updated.lastUpdated = std::time(nullptr);

  db()->updateEntry(updated);

  sync::queue()->pushDelete(animeId);
}

}  // namespace anime::list
