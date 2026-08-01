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

#include <array>
#include <string>

#include "base/chrono.hpp"

namespace anime::list {

enum class Status {
  NotInList,
  Watching,
  Completed,
  OnHold,
  Dropped,
  PlanToWatch,
};

// clang-format off
constexpr std::array<Status, 5> kStatuses{
  Status::Watching,
  Status::Completed,
  Status::OnHold,
  Status::Dropped,
  Status::PlanToWatch,
};
// clang-format on

constexpr int kUnknownId = 0;
constexpr int kScoreMax = 100;

struct Entry {
  int64_t id = kUnknownId;
  int anime_id = kUnknownId;
  int watched_episodes = 0;
  int score = 0;
  Status status = Status::NotInList;
  bool is_private = false;
  int rewatched_times = 0;
  bool rewatching = false;
  int rewatching_ep = 0;
  FuzzyDate date_started;
  FuzzyDate date_completed;
  std::time_t last_updated;
  std::string notes;
  bool pending_delete = false;
};

}  // namespace anime::list

using ListEntry = anime::list::Entry;
