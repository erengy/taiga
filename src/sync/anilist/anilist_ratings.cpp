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

#include "anilist_ratings.hpp"

#include <QList>
#include <QMap>

#include "base/log.hpp"
#include "media/anime_list.hpp"
#include "sync/service.hpp"

namespace sync::anilist {

QList<Rating> ratingList(const RatingSystem ratingSystem) {
  constexpr int k = anime::list::kScoreMax / 100;

  // clang-format off
  switch (ratingSystem) {
    case RatingSystem::Point_100:
      return {
        {  0,       "0"},
        { 10 * k,  "10"},
        { 15 * k,  "15"},
        { 20 * k,  "20"},
        { 25 * k,  "25"},
        { 30 * k,  "30"},
        { 35 * k,  "35"},
        { 40 * k,  "40"},
        { 45 * k,  "45"},
        { 50 * k,  "50"},
        { 55 * k,  "55"},
        { 60 * k,  "60"},
        { 65 * k,  "65"},
        { 70 * k,  "70"},
        { 75 * k,  "75"},
        { 80 * k,  "80"},
        { 85 * k,  "85"},
        { 90 * k,  "90"},
        { 95 * k,  "95"},
        {100 * k, "100"},
      };
      break;
    case RatingSystem::Point_10_Decimal:
      return {
        {  0,       "0"},
        { 10 * k, "1.0"},
        { 15 * k, "1.5"},
        { 20 * k, "2.0"},
        { 25 * k, "2.5"},
        { 30 * k, "3.0"},
        { 35 * k, "3.5"},
        { 40 * k, "4.0"},
        { 45 * k, "4.5"},
        { 50 * k, "5.0"},
        { 55 * k, "5.5"},
        { 60 * k, "6.0"},
        { 65 * k, "6.5"},
        { 70 * k, "7.0"},
        { 75 * k, "7.5"},
        { 80 * k, "8.0"},
        { 85 * k, "8.5"},
        { 90 * k, "9.0"},
        { 95 * k, "9.5"},
        {100 * k,  "10"},
      };
      break;
    case RatingSystem::Point_10:
      return {
        {  0,      "0"},
        { 10 * k,  "1"},
        { 20 * k,  "2"},
        { 30 * k,  "3"},
        { 40 * k,  "4"},
        { 50 * k,  "5"},
        { 60 * k,  "6"},
        { 70 * k,  "7"},
        { 80 * k,  "8"},
        { 90 * k,  "9"},
        {100 * k, "10"},
      };
    case RatingSystem::Point_5:
      return {
        { 0,     "☆☆☆☆☆"},
        {10 * k, "★☆☆☆☆"},
        {30 * k, "★★☆☆☆"},
        {50 * k, "★★★☆☆"},
        {70 * k, "★★★★☆"},
        {90 * k, "★★★★★"},
      };
    case RatingSystem::Point_3:
      return {
        { 0,     "No Score"},
        {35 * k, ":("},
        {60 * k, ":|"},
        {85 * k, ":)"},
      };
  }
  // clang-format on

  return {};
}

QString formatRating(int value, const RatingSystem ratingSystem) {
  value = (value * 100) / anime::list::kScoreMax;

  switch (ratingSystem) {
    case RatingSystem::Point_100:
      return QString::number(value);

    case RatingSystem::Point_10_Decimal:
      return QString::number(value / 10.0, 'g', 1);

    case RatingSystem::Point_10:
      return QString::number(value / 10);

    case RatingSystem::Point_5:
      value = [value]() {
        if (value < 1) return 0;
        if (value < 30) return 1;
        if (value < 50) return 2;
        if (value < 70) return 3;
        if (value < 90) return 4;
        return 5;
      }();
      return QString{u'★'}.repeated(value) + QString{u'☆'}.repeated(5 - value);

    case RatingSystem::Point_3:
      value = [value]() {
        if (value < 1) return 0;
        if (value < 36) return 1;
        if (value < 61) return 2;
        return 3;
      }();
      // clang-format off
      switch (value) {
        default: return "No Score";
        case 1: return ":(";
        case 2: return ":|";
        case 3: return ":)";
      }
      // clang-format on
  }

  LOGW("Invalid value: {}", value);

  return QString::number(value);
}

RatingSystem parseRatingSystem(const QString& value) {
  // clang-format off
  static const QMap<QString, RatingSystem> table{
      {"POINT_100", RatingSystem::Point_100},
      {"POINT_10_DECIMAL", RatingSystem::Point_10_Decimal},
      {"POINT_10", RatingSystem::Point_10},
      {"POINT_5", RatingSystem::Point_5},
      {"POINT_3", RatingSystem::Point_3},
  };
  // clang-format on
  return table.value(value, kDefaultRatingSystem);
}

}  // namespace sync::anilist
