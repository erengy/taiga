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

void SeasonDatabase::Review() {
  const auto [date_start, date_end] = current_season.to_date_range();

  const auto is_within_date_interval =
      [&date_start, &date_end](const Item& anime_item) {
        const auto& anime_start = anime_item.GetDateStart();
        if (anime_start.year() && anime_start.month())
          if (Date{date_start} <= anime_start && anime_start <= Date{date_end})
            return true;
        return false;
      };

  std::set<int> unique_ids;

  // Remove invalid items
  for (size_t i = 0; i < items.size(); ++i) {
    const int anime_id = items.at(i);
    const auto anime_item = anime::db.Find(anime_id);

    const auto remove_item = [&](const std::wstring& reason) {
      items.erase(items.begin() + i--);
      if (taiga::app.options.verbose) {
        LOGD(L"Removed item: #{} \"{}\" ({})", anime_id,
             anime_item ? anime_item->GetTitle() : L"", reason);
      }
    };

    if (!anime_item) {
      remove_item(L"Unavailable");
      continue;
    }
    if (const auto [_, inserted] = unique_ids.insert(anime_id); !inserted) {
      remove_item(L"Duplicate");
      continue;
    }
    if (IsNsfw(*anime_item)) {
      remove_item(L"NSFW");
      continue;
    }
    if (!is_within_date_interval(*anime_item)) {
      remove_item(L"Date: " + anime_item->GetDateStart().to_string());
      continue;
    }
  }

  // Add missing items
  for (const auto& [anime_id, anime_item] : anime::db.items) {
    if (nstd::contains(items, anime_id))
      continue;

    if (IsNsfw(anime_item) || !is_within_date_interval(anime_item))
      continue;

    items.push_back(anime_id);
    if (taiga::app.options.verbose && !unique_ids.empty()) {
      LOGD(L"Added item: #{} \"{}\" ({})", anime_id, anime_item.GetTitle(),
           anime_item.GetDateStart().to_string());
    }
  }
}

}  // namespace anime
