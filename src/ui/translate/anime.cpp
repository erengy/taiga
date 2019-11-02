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
    case sync::kMyAnimeList:
      return ToWstr(value, 2);
    case sync::kKitsu:
      return ToWstr(sync::kitsu::TranslateSeriesRatingTo(value), 2) + L"%";
    case sync::kAniList:
      return ToWstr(sync::anilist::TranslateSeriesRatingTo(value), 0) + L"%";
  }
}

std::wstring TranslateStatus(const int value) {
  switch (value) {
    case anime::kAiring: return L"Currently airing";
    case anime::kFinishedAiring: return L"Finished airing";
    case anime::kNotYetAired: return L"Not yet aired";
    default: return ToWstr(value);
  }
}

std::wstring TranslateType(const int value) {
  switch (value) {
    case anime::kTv: return L"TV";
    case anime::kOva: return L"OVA";
    case anime::kMovie: return L"Movie";
    case anime::kSpecial: return L"Special";
    case anime::kOna: return L"ONA";
    case anime::kMusic: return L"Music";
    default: return L"";
  }
}

int TranslateType(const std::wstring& value) {
  static const std::map<std::wstring, anime::SeriesType> types{
      {L"tv", anime::kTv},
      {L"ova", anime::kOva}, {L"oav", anime::kOva},
      {L"movie", anime::kMovie}, {L"gekijouban", anime::kMovie},
      {L"special", anime::kSpecial},
      {L"ona", anime::kOna},
      {L"music", anime::kMusic},
    };

  auto it = types.find(ToLower_Copy(value));
  return it != types.end() ? it->second : anime::kUnknownType;
}

////////////////////////////////////////////////////////////////////////////////

std::wstring TranslateDateToSeasonString(const Date& date) {
  if (!anime::IsValidDate(date))
    return L"Unknown";

  return TranslateSeason(anime::Season(date));
}

std::wstring TranslateSeasonName(const anime::Season::Name name) {
  switch (name) {
    default:
    case anime::Season::Name::kUnknown: return L"Unknown";
    case anime::Season::Name::kWinter: return L"Winter";
    case anime::Season::Name::kSpring: return L"Spring";
    case anime::Season::Name::kSummer: return L"Summer";
    case anime::Season::Name::kFall: return L"Fall";
  }
}

std::wstring TranslateSeason(const anime::Season& season) {
  return L"{} {}"_format(TranslateSeasonName(season.name), season.year);
}

std::wstring TranslateSeasonToMonths(const anime::Season& season) {
  Date date_start, date_end;
  season.GetInterval(date_start, date_end);
  return TranslateDateRange({date_start, date_end});
}

}  // namespace ui
