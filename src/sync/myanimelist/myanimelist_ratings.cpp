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

#include "myanimelist_ratings.hpp"

#include "media/anime_list.hpp"
#include "sync/myanimelist/myanimelist_utils.hpp"
#include "sync/service.hpp"

namespace sync::myanimelist {

QList<sync::Rating> ratingList() {
  constexpr int k = anime::list::kScoreMax / 10;

  // clang-format off
  return {
      { 0,      "(0) No Score"},
      { 1 * k,  "(1) Appalling"},
      { 2 * k,  "(2) Horrible"},
      { 3 * k,  "(3) Very Bad"},
      { 4 * k,  "(4) Bad"},
      { 5 * k,  "(5) Average"},
      { 6 * k,  "(6) Fine"},
      { 7 * k,  "(7) Good"},
      { 8 * k,  "(8) Very Good"},
      { 9 * k,  "(9) Great"},
      {10 * k, "(10) Masterpiece"},
  };
  // clang-format on
}

QString formatRating(const int value) {
  // clang-format off
  switch (fromListScore(value)) {
    case  0: return  "(0) No Score";
    case  1: return  "(1) Appalling";
    case  2: return  "(2) Horrible";
    case  3: return  "(3) Very Bad";
    case  4: return  "(4) Bad";
    case  5: return  "(5) Average";
    case  6: return  "(6) Fine";
    case  7: return  "(7) Good";
    case  8: return  "(8) Very Good";
    case  9: return  "(9) Great";
    case 10: return "(10) Masterpiece";
  }
  // clang-format on
  return QString::number(value);
}

}  // namespace sync::myanimelist
