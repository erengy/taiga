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

#include <map>

#include "sync/anilist_util.h"

#include "base/file.h"
#include "base/format.h"
#include "base/json.h"
#include "base/log.h"
#include "base/string.h"
#include "media/anime.h"
#include "media/anime_db.h"
#include "sync/anilist.h"
#include "sync/anilist_ratings.h"
#include "taiga/settings.h"

namespace sync::anilist {

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

anime::SeriesStatus TranslateSeriesStatusFrom(const std::string& value) {
  static const std::map<std::string, anime::SeriesStatus> table{
    {"FINISHED", anime::SeriesStatus::FinishedAiring},
    {"RELEASING", anime::SeriesStatus::Airing},
    {"NOT_YET_RELEASED", anime::SeriesStatus::NotYetAired},
    {"CANCELLED", anime::SeriesStatus::NotYetAired},
  };

  const auto it = table.find(value);
  if (it != table.end())
    return it->second;

  if (!value.empty())
    LOGW(L"Invalid value: {}", StrToWstr(value));

  return anime::SeriesStatus::Unknown;
}

double TranslateSeriesRatingFrom(int value) {
  return static_cast<double>(value) / 10.0;
}

double TranslateSeriesRatingTo(double value) {
  return value * 10.0;
}

anime::SeriesType TranslateSeriesTypeFrom(const std::string& value) {
  static const std::map<std::string, anime::SeriesType> table{
    {"TV", anime::SeriesType::Tv},
    {"TV_SHORT", anime::SeriesType::Tv},
    {"MOVIE", anime::SeriesType::Movie},
    {"SPECIAL", anime::SeriesType::Special},
    {"OVA", anime::SeriesType::Ova},
    {"ONA", anime::SeriesType::Ona},
    {"MUSIC", anime::SeriesType::Music},
  };

  const auto it = table.find(value);
  if (it != table.end())
    return it->second;

  if (!value.empty())
    LOGW(L"Invalid value: {}", StrToWstr(value));

  return anime::SeriesType::Unknown;
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
      if (!value) {}
      else if (value < 30) value = 1;
      else if (value < 50) value = 2;
      else if (value < 70) value = 3;
      else if (value < 90) value = 4;
      else value = 5;
      return std::wstring(static_cast<size_t>(    value), L'\u2605') +
             std::wstring(static_cast<size_t>(5 - value), L'\u2606');

    case RatingSystem::Point_3:
      if (!value) {}
      else if (value < 36) value = 1;
      else if (value < 61) value = 2;
      else value = 3;
      switch (value) {
        default: return L"No Score";
        case 1: return L":(";
        case 2: return L":|";
        case 3: return L":)";
      }
  }

  LOGW(L"Invalid value: {}", value);
  return ToWstr(value);
}

anime::MyStatus TranslateMyStatusFrom(const std::string& value) {
  static const std::map<std::string, anime::MyStatus> table{
    {"CURRENT", anime::MyStatus::Watching},
    {"PLANNING", anime::MyStatus::PlanToWatch},
    {"COMPLETED", anime::MyStatus::Completed},
    {"DROPPED", anime::MyStatus::Dropped},
    {"PAUSED", anime::MyStatus::OnHold},
  };

  const auto it = table.find(value);
  if (it != table.end())
    return it->second;

  LOGW(L"Invalid value: {}", StrToWstr(value));
  return anime::MyStatus::NotInList;
}

std::string TranslateMyStatusTo(anime::MyStatus value) {
  switch (value) {
    case anime::MyStatus::Watching: return "CURRENT";
    case anime::MyStatus::Completed: return "COMPLETED";
    case anime::MyStatus::OnHold: return "PAUSED";
    case anime::MyStatus::Dropped: return "DROPPED";
    case anime::MyStatus::PlanToWatch: return "PLANNING";
  }

  LOGW(L"Invalid value: {}", static_cast<int>(value));
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

std::wstring GetAnimePage(const anime::Item& anime_item) {
  return L"https://anilist.co/anime/{}"_format(anime_item.GetId());
}

void RequestToken() {
  constexpr auto kTaigaClientId = 161;
  ExecuteLink(L"https://anilist.co/api/v2/oauth/authorize"
              L"?client_id={}&response_type=token"_format(kTaigaClientId));
}

void ViewAnimePage(int anime_id) {
  if (const auto anime_item = anime::db.Find(anime_id))
    ExecuteLink(GetAnimePage(*anime_item));
}

void ViewProfile() {
  ExecuteLink(L"https://anilist.co/user/{}"_format(
      taiga::settings.GetSyncServiceAniListUsername()));
}

void ViewStats() {
  ExecuteLink(L"https://anilist.co/user/{}/stats"_format(
      taiga::settings.GetSyncServiceAniListUsername()));
}

}  // namespace sync::anilist
