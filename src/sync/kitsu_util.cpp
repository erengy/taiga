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
#include "base/log.h"
#include "base/string.h"
#include "base/time.h"
#include "library/anime.h"
#include "library/anime_db.h"
#include "sync/kitsu_util.h"
#include "sync/kitsu_types.h"
#include "taiga/settings.h"

namespace sync {
namespace kitsu {

std::wstring DecodeSynopsis(std::string text) {
  auto str = StrToWstr(text);
  ReplaceString(str, L"\n\n", L"\r\n\r\n");
  return str;
}

////////////////////////////////////////////////////////////////////////////////

int TranslateAgeRatingFrom(const std::string& value) {
  static const std::map<std::string, anime::AgeRating> table{
    {"G", anime::kAgeRatingG},
    {"PG", anime::kAgeRatingPG},
    {"R", anime::kAgeRatingR17},
    {"R18", anime::kAgeRatingR18},
  };

  const auto it = table.find(value);
  if (it != table.end())
    return it->second;

  if (!value.empty())
    LOGD(L"Invalid value: " + StrToWstr(value));

  return anime::kUnknownAgeRating;
}

double TranslateSeriesRatingFrom(const std::string& value) {
  return ToDouble(value) / 10.0;
}

double TranslateSeriesRatingTo(double value) {
  return value * 10.0;
}

int TranslateSeriesTypeFrom(const std::string& value) {
  static const std::map<std::string, anime::SeriesType> table{
    {"TV", anime::kTv},
    {"special", anime::kSpecial},
    {"OVA", anime::kOva},
    {"ONA", anime::kOna},
    {"movie", anime::kMovie},
    {"music", anime::kMusic},
  };

  const auto it = table.find(value);
  if (it != table.end())
    return it->second;

  LOGW(L"Invalid value: " + StrToWstr(value));
  return anime::kUnknownType;
}

std::wstring TranslateMyLastUpdatedFrom(const std::string& value) {
  // Get Unix time from ISO 8601
  const auto result = ConvertIso8601(StrToWstr(value));
  return result != -1 ? ToWstr(result) : std::wstring();
}

int TranslateMyRatingFrom(const std::string& value) {
  return static_cast<int>(ToDouble(value) * 2.0);
}

std::string TranslateMyRatingTo(int value) {
  return ToStr(static_cast<double>(value) / 2.0, 1);
}

int TranslateMyStatusFrom(const std::string& value) {
  static const std::map<std::string, anime::MyStatus> table{
    {"current", anime::kWatching},
    {"planned", anime::kPlanToWatch},
    {"completed", anime::kCompleted},
    {"on_hold", anime::kOnHold},
    {"dropped", anime::kDropped},
  };

  const auto it = table.find(value);
  if (it != table.end())
    return it->second;

  LOGW(L"Invalid value: " + StrToWstr(value));
  return anime::kNotInList;
}

std::string TranslateMyStatusTo(int value) {
  switch (value) {
    case anime::kWatching: return "current";
    case anime::kCompleted: return "completed";
    case anime::kOnHold: return "on_hold";
    case anime::kDropped: return "dropped";
    case anime::kPlanToWatch: return "planned";
  }

  LOGW(L"Invalid value: " + ToWstr(value));
  return "";
}

////////////////////////////////////////////////////////////////////////////////

static const std::wstring kBaseUrl = L"https://kitsu.io";

std::wstring GetAnimePage(const anime::Item& anime_item) {
  return kBaseUrl + L"/anime/" + anime_item.GetSlug();
}

void ViewAnimePage(int anime_id) {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (anime_item)
    ExecuteLink(GetAnimePage(*anime_item));
}

void ViewFeed() {
  ExecuteLink(kBaseUrl);
}

void ViewLibrary() {
  ExecuteLink(kBaseUrl + L"/users/" +
              Settings[taiga::kSync_Service_Kitsu_Username] + L"/library");
}

void ViewProfile() {
  ExecuteLink(kBaseUrl + L"/users/" +
              Settings[taiga::kSync_Service_Kitsu_Username]);
}

}  // namespace kitsu
}  // namespace sync
