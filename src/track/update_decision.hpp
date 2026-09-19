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

#pragma once

#include <optional>

#include "media/anime.hpp"
#include "media/anime_list.hpp"
#include "track/episode.hpp"

namespace track {

struct UpdateDecision {
  enum class Action {
    Allow,    // update automatically
    Confirm,  // update only if user agrees
    Deny,     // don't update
  };

  enum class Reason {
    None,
    InvalidEpisode,
    OutsideLibrary,
    AlreadyWatched,
    SkipsAhead,
  };

  Action action = Action::Allow;
  Reason reason = Reason::None;
};

std::optional<int> watchedEpisodeNumber(const Episode& episode, const anime::Details& item);

UpdateDecision decideUpdate(const Episode& episode, const anime::Details& item,
                            const ListEntry* entry, const bool outsideLibrary);

}  // namespace track
