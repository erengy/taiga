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

#include "kitsu_ratings.hpp"

#include <QList>
#include <QMap>
#include <QString>

#include "base/log.hpp"
#include "media/anime_list.hpp"
#include "sync/service.hpp"

namespace sync::kitsu {

QList<Rating> ratingList(const RatingSystem ratingSystem) {
  constexpr int k = anime::list::kScoreMax / 20;

  // clang-format off
  switch (ratingSystem) {
    case RatingSystem::Simple:
      return {
        { 0,     "-"},
        { 2 * k, "Awful"},
        { 8 * k, "Meh"},
        {14 * k, "Good"},
        {20 * k, "Great"},
      };

    case RatingSystem::Regular:
      return {
        { 0,     "★ 0.0"},
        { 2 * k, "★ 0.5"},
        { 4 * k, "★ 1.0"},
        { 6 * k, "★ 1.5"},
        { 8 * k, "★ 2.0"},
        {10 * k, "★ 2.5"},
        {12 * k, "★ 3.0"},
        {14 * k, "★ 3.5"},
        {16 * k, "★ 4.0"},
        {18 * k, "★ 4.5"},
        {20 * k, "★ 5.0"},
      };

    case RatingSystem::Advanced:
      return {
        { 0,      "0.0"},
        { 2 * k,  "1.0"},
        { 3 * k,  "1.5"},
        { 4 * k,  "2.0"},
        { 5 * k,  "2.5"},
        { 6 * k,  "3.0"},
        { 7 * k,  "3.5"},
        { 8 * k,  "4.0"},
        { 9 * k,  "4.5"},
        {10 * k,  "5.0"},
        {11 * k,  "5.5"},
        {12 * k,  "6.0"},
        {13 * k,  "6.5"},
        {14 * k,  "7.0"},
        {15 * k,  "7.5"},
        {16 * k,  "8.0"},
        {17 * k,  "8.5"},
        {18 * k,  "9.0"},
        {19 * k,  "9.5"},
        {20 * k, "10.0"},
      };
  }
  // clang-format on

  return {};
}

QString formatRating(int value, const RatingSystem ratingSystem) {
  value = (value * 20) / anime::list::kScoreMax;

  switch (ratingSystem) {
    case RatingSystem::Simple:
      // clang-format off
      switch (static_cast<int>(std::ceil(value / 5.0))) {
        case 0: return "-";
        case 1: return "Awful";
        case 2: return "Meh";
        case 3: return "Good";
        case 4: return "Great";
      }
      // clang-format on
      break;

    case RatingSystem::Regular:
      value = value / 2;
      return "★ " + QString::number(value / 2.0, 'f', 1);

    case RatingSystem::Advanced:
      if (value == 1) break;  // there is no "0.5" rating
      return QString::number(value / 2.0, 'f', 1);
  }

  LOGW("Invalid value: {}", value);

  return QString::number(value);
}

int parseListScore(const int value) {
  return value * (anime::list::kScoreMax / 20);
}

int fromListScore(const int value) {
  return (value * 20) / anime::list::kScoreMax;
}

RatingSystem parseRatingSystem(const QString& value) {
  static const QMap<QString, RatingSystem> table{
      {"simple", RatingSystem::Simple},
      {"regular", RatingSystem::Regular},
      {"advanced", RatingSystem::Advanced},
  };
  return table.value(value, kDefaultRatingSystem);
}

}  // namespace sync::kitsu
