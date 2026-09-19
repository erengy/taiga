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

#include "update_decision.hpp"

#include "media/anime_list_utils.hpp"

namespace track {

std::optional<int> watchedEpisodeNumber(const Episode& episode, const anime::Details& item) {
  const auto range = episode.episodeNumberRange();

  if (range && range->second > 0) return range->second;

  // Single-episode anime can do without an episode number.
  if (item.episode_count == 1) return 1;

  return std::nullopt;
}

UpdateDecision decideUpdate(const Episode& episode, const anime::Details& item,
                            const ListEntry* entry, const bool outsideLibrary) {
  using Action = UpdateDecision::Action;
  using Reason = UpdateDecision::Reason;
  using Status = anime::list::Status;

  if (outsideLibrary) return {Action::Deny, Reason::OutsideLibrary};

  const auto number = watchedEpisodeNumber(episode, item);
  if (!number || (item.episode_count > 0 && *number > item.episode_count)) {
    return {Action::Deny, Reason::InvalidEpisode};
  }

  const bool inList = anime::list::isInList(entry);
  const bool completed = inList && entry->status == Status::Completed && !entry->rewatching;
  const int watchedEpisodes = inList ? entry->watched_episodes : 0;

  if (completed || *number < watchedEpisodes ||
      (*number == watchedEpisodes && item.episode_count != 1)) {
    return {Action::Deny, Reason::AlreadyWatched};
  }

  const auto range = episode.episodeNumberRange();
  const int lowestNumber = range ? range->first : *number;
  if (lowestNumber > watchedEpisodes + 1) {
    return {Action::Confirm, Reason::SkipsAhead};
  }

  return {Action::Allow, Reason::None};
}

}  // namespace track
