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

#include "sync/anilist_ratings.h"

#include "base/string.h"
#include "media/anime.h"
#include "sync/anilist_util.h"
#include "taiga/settings.h"

namespace sync::anilist {

RatingSystem GetRatingSystem() {
  return TranslateRatingSystemFrom(
      WstrToStr(taiga::settings.GetSyncServiceAniListRatingSystem()));
}

std::vector<Rating> GetMyRatings(const RatingSystem rating_system) {
  constexpr int k = anime::kUserScoreMax / 100;

  switch (rating_system) {
    case RatingSystem::Point_100:
      return {
        {  0,       L"0"},
        { 10 * k,  L"10"},
        { 15 * k,  L"15"},
        { 20 * k,  L"20"},
        { 25 * k,  L"25"},
        { 30 * k,  L"30"},
        { 35 * k,  L"35"},
        { 40 * k,  L"40"},
        { 45 * k,  L"45"},
        { 50 * k,  L"50"},
        { 55 * k,  L"55"},
        { 60 * k,  L"60"},
        { 65 * k,  L"65"},
        { 70 * k,  L"70"},
        { 75 * k,  L"75"},
        { 80 * k,  L"80"},
        { 85 * k,  L"85"},
        { 90 * k,  L"90"},
        { 95 * k,  L"95"},
        {100 * k, L"100"},
      };
      break;
    case RatingSystem::Point_10_Decimal:
      return {
        {  0,       L"0"},
        { 10 * k, L"1.0"},
        { 15 * k, L"1.5"},
        { 20 * k, L"2.0"},
        { 25 * k, L"2.5"},
        { 30 * k, L"3.0"},
        { 35 * k, L"3.5"},
        { 40 * k, L"4.0"},
        { 45 * k, L"4.5"},
        { 50 * k, L"5.0"},
        { 55 * k, L"5.5"},
        { 60 * k, L"6.0"},
        { 65 * k, L"6.5"},
        { 70 * k, L"7.0"},
        { 75 * k, L"7.5"},
        { 80 * k, L"8.0"},
        { 85 * k, L"8.5"},
        { 90 * k, L"9.0"},
        { 95 * k, L"9.5"},
        {100 * k,  L"10"},
      };
      break;
    case RatingSystem::Point_10:
      return {
        {  0,      L"0"},
        { 10 * k,  L"1"},
        { 20 * k,  L"2"},
        { 30 * k,  L"3"},
        { 40 * k,  L"4"},
        { 50 * k,  L"5"},
        { 60 * k,  L"6"},
        { 70 * k,  L"7"},
        { 80 * k,  L"8"},
        { 90 * k,  L"9"},
        {100 * k, L"10"},
      };
    case RatingSystem::Point_5:
      return {
        { 0,     L"\u2606\u2606\u2606\u2606\u2606"},
        {10 * k, L"\u2605\u2606\u2606\u2606\u2606"},
        {30 * k, L"\u2605\u2605\u2606\u2606\u2606"},
        {50 * k, L"\u2605\u2605\u2605\u2606\u2606"},
        {70 * k, L"\u2605\u2605\u2605\u2605\u2606"},
        {90 * k, L"\u2605\u2605\u2605\u2605\u2605"},
      };
    case RatingSystem::Point_3:
      return {
        { 0,     L"No Score"},
        {35 * k, L":("},
        {60 * k, L":|"},
        {85 * k, L":)"},
      };
  }

  return {};
}

}  // namespace sync::anilist
