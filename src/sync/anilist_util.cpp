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
#include "sync/anilist.h"
#include "sync/anilist_types.h"
#include "sync/anilist_util.h"
#include "sync/manager.h"
#include "taiga/settings.h"

namespace sync {
namespace anilist {

std::wstring DecodeDescription(std::string text) {
  auto str = StrToWstr(text);
  ReplaceString(str, L"\n", L"\r\n");
  ReplaceString(str, L"<br>", L"\r\n");
  ReplaceString(str, L"\r\n\r\n\r\n", L"\r\n\r\n");
  return str;
}
////////////////////////////////////////////////////////////////////////////////

RatingSystem GetRatingSystem() {
  const auto& service = *ServiceManager.service(sync::kAniList);
  return TranslateRatingSystemFrom(WstrToStr(service.user().rating_system));
}

std::vector<Rating> GetMyRatings(RatingSystem rating_system) {
  constexpr int k = anime::kUserScoreMax / 100;

  switch (rating_system) {
    case RatingSystem::Point_100:
      // TODO
      break;
    case RatingSystem::Point_10_Decimal:
      // TODO
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
        {  0,     L"\u2606\u2606\u2606\u2606\u2606"},
        { 20 * k, L"\u2605\u2606\u2606\u2606\u2606"},
        { 40 * k, L"\u2605\u2605\u2606\u2606\u2606"},
        { 60 * k, L"\u2605\u2605\u2605\u2606\u2606"},
        { 80 * k, L"\u2605\u2605\u2605\u2605\u2606"},
        {100 * k, L"\u2605\u2605\u2605\u2605\u2605"},
      };
    case RatingSystem::Point_3:
      // TODO
      return {
        {0,     L"No Score"},
        {1 * k, L":("},
        {2 * k, L":|"},
        {3 * k, L":)"},
      };
  }

  return {};
}

////////////////////////////////////////////////////////////////////////////////

Date TranslateFuzzyDateFrom(const Json& json) {
  return Date{
      static_cast<unsigned short>(JsonReadInt(json, "year")),
      static_cast<unsigned short>(JsonReadInt(json, "month")),
      static_cast<unsigned short>(JsonReadInt(json, "day"))
  };
}

Json TranslateFuzzyDateTo(const Date& date) {
  Json json{{"year", nullptr}, {"month", nullptr}, {"day", nullptr}};

  if (date.year()) json["year"] = date.year();
  if (date.month()) json["month"] = date.month();
  if (date.day()) json["day"] = date.day();

  return json;
}

std::string TranslateSeasonTo(const std::wstring& value) {
  return WstrToStr(ToUpper_Copy(value));
}

double TranslateSeriesRatingFrom(int value) {
  return static_cast<double>(value) / 10.0;
}

double TranslateSeriesRatingTo(double value) {
  return value * 10.0;
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

std::wstring TranslateMyRating(int value, RatingSystem rating_system) {
  value = (value * 100) / anime::kUserScoreMax;

  switch (rating_system) {
    case RatingSystem::Point_100:
      return ToWstr(value);
    case RatingSystem::Point_10_Decimal:
      return ToWstr(static_cast<double>(value) / 10, 1);
    case RatingSystem::Point_10:
      return ToWstr(value / 10);
    case RatingSystem::Point_5:
      value = value / 20;
      return std::wstring(static_cast<size_t>(    value), L'\u2605') +
             std::wstring(static_cast<size_t>(5 - value), L'\u2606');
    case RatingSystem::Point_3:
      switch (value % 30) {
        default: return L"No Score";
        case 1: return L":(";
        case 2: return L":|";
        case 3: return L":)";
      }
  }

  LOGW(L"Invalid value: {}", value);
  return ToWstr(value);
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

std::string TranslateMyStatusTo(int value) {
  switch (value) {
    case anime::kWatching: return "CURRENT";
    case anime::kCompleted: return "COMPLETED";
    case anime::kOnHold: return "PAUSED";
    case anime::kDropped: return "DROPPED";
    case anime::kPlanToWatch: return "PLANNING";
  }

  LOGW(L"Invalid value: {}", value);
  return "";
}

RatingSystem TranslateRatingSystemFrom(const std::string& value) {
  static const std::map<std::string, RatingSystem> table{
    {"POINT_100", RatingSystem::Point_100},
    {"POINT_10_DECIMAL", RatingSystem::Point_10_Decimal},
    {"POINT_10", RatingSystem::Point_10},
    {"POINT_5", RatingSystem::Point_5},
    {"POINT_3", RatingSystem::Point_3},
  };

  const auto it = table.find(value);
  if (it != table.end())
    return it->second;

  LOGW(L"Invalid value: {}", StrToWstr(value));
  return kDefaultRatingSystem;
}

////////////////////////////////////////////////////////////////////////////////

static const std::wstring kBaseUrl = L"https://anilist.co";

std::wstring GetAnimePage(const anime::Item& anime_item) {
  return L"{}/anime/{}"_format(kBaseUrl, anime_item.GetId());
}

void RequestToken() {
  constexpr auto kTaigaClientId = 161;
  ExecuteLink(L"https://anilist.co/api/v2/oauth/authorize"
              L"?client_id={}&response_type=token"_format(kTaigaClientId));
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
