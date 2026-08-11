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

#include <QList>
#include <optional>
#include <string>

#include "media/anime_list.hpp"

namespace anime {
struct HistoryItem;
}

namespace compat::v1 {

struct QueueItem {
  int anime_id = 0;
  bool delete_entry = false;
  std::optional<int> episode;
  std::optional<int> score;
  std::optional<anime::list::Status> status;
  std::optional<bool> rewatching;
  std::optional<int> rewatched_times;
  std::optional<std::string> notes;
  std::optional<FuzzyDate> date_started;
  std::optional<FuzzyDate> date_completed;
};

QList<anime::HistoryItem> readHistory(const std::string& path);
QList<QueueItem> readQueue(const std::string& path);

}  // namespace compat::v1
