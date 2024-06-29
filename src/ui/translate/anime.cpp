/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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

#include <map>
#include <string>

#include "ui/translate.h"

#include "base/format.h"
#include "base/string.h"
#include "media/anime.h"
#include "media/anime_season.h"
#include "media/anime_util.h"
#include "sync/anilist_util.h"
#include "sync/kitsu_util.h"
#include "sync/myanimelist_util.h"
#include "sync/service.h"
#include "taiga/settings.h"

namespace ui {

std::wstring TranslateScore(const double value) {
  switch (sync::GetCurrentServiceId()) {
    default:
    case sync::ServiceId::MyAnimeList:
      return ToWstr(value, 2);
    case sync::ServiceId::Kitsu:
      return ToWstr(sync::kitsu::TranslateSeriesRatingTo(value), 2) + L"%";
    case sync::ServiceId::AniList:
      return ToWstr(sync::anilist::TranslateSeriesRatingTo(value), 0) + L"%";
  }
}

anime::SeriesType TranslateType(const std::wstring& value) {
  static const std::map<std::wstring, anime::SeriesType> types{
      {L"tv", anime::SeriesType::Tv},
      {L"ova", anime::SeriesType::Ova},
      {L"oav", anime::SeriesType::Ova},
      {L"movie", anime::SeriesType::Movie},
      {L"gekijouban", anime::SeriesType::Movie},
      {L"special", anime::SeriesType::Special},
      {L"ona", anime::SeriesType::Ona},
      {L"music", anime::SeriesType::Music},
    };

  auto it = types.find(ToLower_Copy(value));
  return it != types.end() ? it->second : anime::SeriesType::Unknown;
}

////////////////////////////////////////////////////////////////////////////////

std::wstring TranslateDateToSeasonString(const Date& date) {
  if (!anime::IsValidDate(date))
    return L"Unknown";

  return TranslateSeason(anime::Season(date));
}

std::wstring TranslateSeasonToMonths(const anime::Season& season) {
  const auto [date_start, date_end] = season.to_date_range();
  return TranslateDateRange({Date{date_start}, Date{date_end}});
}

}  // namespace ui
