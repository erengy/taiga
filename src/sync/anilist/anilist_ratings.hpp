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
#include <QString>

namespace sync {
struct Rating;
}

namespace sync::anilist {

enum class RatingSystem {
  Point100,
  Point10Decimal,
  Point10,
  Point5,
  Point3,
};

constexpr auto kDefaultRatingSystem = RatingSystem::Point10;

QList<sync::Rating> ratingList(const RatingSystem ratingSystem);

QString formatRating(int value, const RatingSystem ratingSystem);

RatingSystem parseRatingSystem(const QString& value);

}  // namespace sync::anilist
