/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#include "base/file.h"
#include "base/format.h"
#include "base/json.h"
#include "base/log.h"
#include "base/string.h"
#include "library/anime.h"
#include "library/anime_db.h"
#include "taiga/settings.h"

namespace sync {
namespace anilist {

std::wstring DecodeDescription(std::string text) {
  auto str = StrToWstr(text);
  ReplaceString(str, L"<br>", L"\r\n");
  ReplaceString(str, L"\r\n\r\n\r\n", L"\r\n\r\n");
  return str;
}

////////////////////////////////////////////////////////////////////////////////

Date TranslateFuzzyDateFrom(const Json& json) {
  return Date{
      static_cast<unsigned short>(JsonReadInt(json, "year")),
      static_cast<unsigned short>(JsonReadInt(json, "month")),
      static_cast<unsigned short>(JsonReadInt(json, "day"))
  };
}

int TranslateSeriesTypeFrom(const std::string& value) {
  static const std::map<std::string, anime::SeriesType> table{
    {"TV", anime::kTv},
    {"TV_SHORT", anime::kTv},
    {"MOVIE", anime::kMovie},
    {"SPECIAL", anime::kSpecial},
    {"OVA", anime::kOva},
    {"ONA", anime::kOna},
    {"MUSIC", anime::kMusic},
  };

  const auto it = table.find(value);
  if (it != table.end())
    return it->second;

  LOGW(L"Invalid value: {}", StrToWstr(value));
  return anime::kUnknownType;
}

int TranslateMyStatusFrom(const std::string& value) {
  static const std::map<std::string, anime::MyStatus> table{
    {"CURRENT", anime::kWatching},
    {"PLANNING", anime::kPlanToWatch},
    {"COMPLETED", anime::kCompleted},
    {"DROPPED", anime::kDropped},
    {"PAUSED", anime::kOnHold},
  };

  const auto it = table.find(value);
  if (it != table.end())
    return it->second;

  LOGW(L"Invalid value: {}", StrToWstr(value));
  return anime::kNotInList;
}

////////////////////////////////////////////////////////////////////////////////

static const std::wstring kBaseUrl = L"https://anilist.co";

std::wstring GetAnimePage(const anime::Item& anime_item) {
  return L"{}/anime/{}"_format(kBaseUrl, anime_item.GetId());
}

void ViewAnimePage(int anime_id) {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (anime_item)
    ExecuteLink(GetAnimePage(*anime_item));
}

void ViewProfile() {
  ExecuteLink(L"{}/user/{}"_format(kBaseUrl,
              Settings[taiga::kSync_Service_AniList_Username]));
}

void ViewStats() {
  ExecuteLink(L"{}/user/{}/stats"_format(kBaseUrl,
              Settings[taiga::kSync_Service_AniList_Username]));
}

}  // namespace anilist
}  // namespace sync
