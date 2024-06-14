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

#pragma once

#include <vector>

namespace sync {
struct Rating;
}

namespace sync::anilist {

enum class RatingSystem {
  Point_100,
  Point_10_Decimal,
  Point_10,
  Point_5,
  Point_3,
};

constexpr auto kDefaultRatingSystem = RatingSystem::Point_10;

RatingSystem GetRatingSystem();
std::vector<sync::Rating> GetMyRatings(const RatingSystem rating_system);

}  // namespace sync::anilist
