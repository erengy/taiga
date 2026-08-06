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

#include <QString>
#include <optional>

#include "media/anime.hpp"
#include "media/anime_season.hpp"

namespace sync {

enum class SearchSort {
  Title,
  Duration,
  Score,
  Type,
  StartDate,
};

struct SearchParams {
  QString text;
  std::optional<int> year;
  std::optional<anime::SeasonName> season;
  std::optional<anime::Type> type;
  std::optional<anime::Status> status;
  std::optional<SearchSort> sort;
  Qt::SortOrder sortOrder = Qt::SortOrder::AscendingOrder;

  bool operator==(const SearchParams&) const = default;
};

}  // namespace sync
