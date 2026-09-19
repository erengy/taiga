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

#include <chrono>

#include "track/update_decision.hpp"

namespace track {

struct UpdateState {
  enum class Phase {
    Idle,             // nothing to update
    Denied,           // update is not allowed, see `reason`
    Countdown,        // list will be updated automatically after `remaining`
    WaitingForClose,  // list will be updated automatically when media is closed
    Confirming,       // list will be updated if user agrees, see `reason`
    Committed,        // list has been updated
    Cancelled,        // user has declined the update
  };

  Phase phase = Phase::Idle;
  UpdateDecision::Reason reason = UpdateDecision::Reason::None;
  std::chrono::seconds remaining{0};
  int episode = 0;
  int previousEpisode = 0;
  bool paused = false;  // countdown is not running, because media player is not in focus

  bool operator==(const UpdateState&) const = default;
};

}  // namespace track
