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

#include <QList>
#include <QString>

namespace sync {
struct Rating;
}

namespace sync::kitsu {

enum class RatingSystem {
  Simple,
  Regular,
  Advanced,
};

constexpr auto kDefaultRatingSystem = RatingSystem::Regular;

QList<Rating> ratingList(const RatingSystem ratingSystem);
QString formatRating(int value, const RatingSystem ratingSystem);
int parseListScore(const int value);
int fromListScore(const int value);
RatingSystem parseRatingSystem(const QString& value);

}  // namespace sync::kitsu
