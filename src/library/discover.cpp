/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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

#include <algorithm>

#include "base/file.h"
#include "base/format.h"
#include "base/log.h"
#include "base/string.h"
#include "base/xml.h"
#include "library/anime_db.h"
#include "library/anime_item.h"
#include "library/anime_season.h"
#include "library/anime_util.h"
#include "library/discover.h"
#include "sync/manager.h"
#include "sync/service.h"
#include "taiga/http.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "ui/dlg/dlg_season.h"
#include "ui/translate.h"
#include "ui/ui.h"

library::SeasonDatabase SeasonDatabase;

namespace library {

bool SeasonDatabase::Load(const anime::Season& season) {
  current_season = season;

  items.clear();
  Review();

  return true;
}

bool SeasonDatabase::IsRefreshRequired() {
  int count = 0;
  bool required = false;

  for (const auto& anime_id : items) {
    auto anime_item = anime::db.FindItem(anime_id);
    if (anime_item) {
      const Date& date_start = anime_item->GetDateStart();
      if (!anime::IsValidDate(date_start) || anime_item->GetSynopsis().empty())
        count++;
    }
    if (count > 20) {
      required = true;
      break;
    }
  }

  return required;
}

void SeasonDatabase::Reset() {
  items.clear();

  current_season.name = anime::Season::kUnknown;
  current_season.year = 0;
}

void SeasonDatabase::Review(bool hide_nsfw) {
  Date date_start, date_end;
  current_season.GetInterval(date_start, date_end);

  const auto is_within_date_interval =
      [&date_start, &date_end](const anime::Item& anime_item) {
        const Date& anime_start = anime_item.GetDateStart();
        if (anime_start.year() && anime_start.month())
          if (date_start <= anime_start && anime_start <= date_end)
            return true;
        return false;
      };

  const auto is_nsfw =
      [&hide_nsfw](const anime::Item& anime_item) {
        return hide_nsfw && IsNsfw(anime_item);
      };

  // Check for invalid items
  for (size_t i = 0; i < items.size(); i++) {
    const int anime_id = items.at(i);
    auto anime_item = anime::db.FindItem(anime_id);
    if (anime_item) {
      const Date& anime_start = anime_item->GetDateStart();
      if (is_nsfw(*anime_item) ||
          (anime::IsValidDate(anime_start) && !is_within_date_interval(*anime_item))) {
        items.erase(items.begin() + i--);
        LOGD(L"Removed item: #{} \"{}\" ({})", anime_id,
             anime_item->GetTitle(), anime_start.to_string());
      }
    }
  }

  // Check for missing items
  for (const auto& [anime_id, anime_item] : anime::db.items) {
    if (std::find(items.begin(), items.end(), anime_id) != items.end())
      continue;
    if (is_nsfw(anime_item) || !is_within_date_interval(anime_item))
      continue;
    items.push_back(anime_id);
    switch (taiga::GetCurrentServiceId()) {
      default:
        LOGD(L"Added item: #{} \"{}\" ({})", anime_id,
             anime_item.GetTitle(), anime_item.GetDateStart().to_string());
        break;
      case sync::kMyAnimeList:
        LOGD(L"\t<anime>\n"
             L"\t\t<type>" + ToWstr(anime_item.GetType()) + L"</type>\n"
             L"\t\t<id name=\"myanimelist\">" + ToWstr(anime_id) + L"</id>\n"
             L"\t\t<producers>" + Join(anime_item.GetProducers(), L", ") + L"</producers>\n"
             L"\t\t<image>" + anime_item.GetImageUrl() + L"</image>\n"
             L"\t\t<title>" + anime_item.GetTitle() + L"</title>\n"
             L"\t</anime>\n");
        break;
    }
  }
}

}  // namespace library
