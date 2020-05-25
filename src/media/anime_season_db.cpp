/*
** Taiga
** Copyright (C) 2010-2020, Eren Okka
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <nstd/algorithm.hpp>

#include "media/anime_season_db.h"

#include "base/log.h"
#include "base/string.h"
#include "media/anime_db.h"
#include "media/anime_item.h"
#include "media/anime_util.h"
#include "taiga/app.h"

namespace anime {

bool SeasonDatabase::IsRefreshRequired() const {
  int count = 0;

  for (const auto& anime_id : items) {
    if (const auto anime_item = anime::db.Find(anime_id)) {
      const Date& date_start = anime_item->GetDateStart();
      if (!IsValidDate(date_start) || anime_item->GetSynopsis().empty()) {
        if (++count > 20) {
          return true;
        }
      }
    }
  }

  return false;
}

void SeasonDatabase::Set(const Season& season) {
  current_season = season;
  items.clear();
  Review();
}

void SeasonDatabase::Reset() {
  current_season = Season{};
  items.clear();
}

void SeasonDatabase::Review(bool hide_nsfw) {
  const auto [date_start, date_end] = current_season.to_date_range();

  const auto is_within_date_interval =
      [&date_start, &date_end](const Item& anime_item) {
        const auto& anime_start = anime_item.GetDateStart();
        if (anime_start.year() && anime_start.month())
          if (Date{date_start} <= anime_start && anime_start <= Date{date_end})
            return true;
        return false;
      };

  const auto is_nsfw = [&hide_nsfw](const Item& anime_item) {
    return hide_nsfw && IsNsfw(anime_item);
  };

  // Check for invalid items
  for (size_t i = 0; i < items.size(); ++i) {
    const int anime_id = items.at(i);
    if (const auto anime_item = anime::db.Find(anime_id)) {
      const Date& anime_start = anime_item->GetDateStart();
      if (is_nsfw(*anime_item) ||
          (IsValidDate(anime_start) && !is_within_date_interval(*anime_item))) {
        items.erase(items.begin() + i--);
        if (taiga::app.options.verbose) {
          LOGD(L"Removed item: #{} \"{}\" ({})", anime_id,
               anime_item->GetTitle(), anime_start.to_string());
        }
      }
    }
  }

  // Check for missing items
  for (const auto& [anime_id, anime_item] : anime::db.items) {
    if (nstd::contains(items, anime_id))
      continue;
    if (is_nsfw(anime_item) || !is_within_date_interval(anime_item))
      continue;
    items.push_back(anime_id);
    if (taiga::app.options.verbose) {
      LOGD(L"Added item: #{} \"{}\" ({})", anime_id, anime_item.GetTitle(),
           anime_item.GetDateStart().to_string());
    }
  }
}

}  // namespace anime
