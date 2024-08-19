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

#include "sync/kitsu_util.h"

#include "base/file.h"
#include "base/format.h"
#include "base/log.h"
#include "base/string.h"
#include "base/time.h"
#include "media/anime.h"
#include "media/anime_db.h"
#include "sync/kitsu.h"
#include "sync/kitsu_types.h"
#include "taiga/settings.h"

namespace sync::kitsu {

RatingSystem GetRatingSystem() {
  return TranslateRatingSystemFrom(
      WstrToStr(taiga::settings.GetSyncServiceKitsuRatingSystem()));
}

std::vector<Rating> GetMyRatings(RatingSystem rating_system) {
  constexpr int k = anime::kUserScoreMax / 20;

  switch (rating_system) {
    case RatingSystem::Simple:
      return {
        { 0,     L"-"},
        { 2 * k, L"Awful"},
        { 8 * k, L"Meh"},
        {14 * k, L"Good"},
        {20 * k, L"Great"},
      };

    case RatingSystem::Regular:
      return {
        { 0,     L"\u2605 0.0"},
        { 2 * k, L"\u2605 0.5"},
        { 4 * k, L"\u2605 1.0"},
        { 6 * k, L"\u2605 1.5"},
        { 8 * k, L"\u2605 2.0"},
        {10 * k, L"\u2605 2.5"},
        {12 * k, L"\u2605 3.0"},
        {14 * k, L"\u2605 3.5"},
        {16 * k, L"\u2605 4.0"},
        {18 * k, L"\u2605 4.5"},
        {20 * k, L"\u2605 5.0"},
      };

    case RatingSystem::Advanced:
      return {
        { 0,      L"0.0"},
        { 2 * k,  L"1.0"},
        { 3 * k,  L"1.5"},
        { 4 * k,  L"2.0"},
        { 5 * k,  L"2.5"},
        { 6 * k,  L"3.0"},
        { 7 * k,  L"3.5"},
        { 8 * k,  L"4.0"},
        { 9 * k,  L"4.5"},
        {10 * k,  L"5.0"},
        {11 * k,  L"5.5"},
        {12 * k,  L"6.0"},
        {13 * k,  L"6.5"},
        {14 * k,  L"7.0"},
        {15 * k,  L"7.5"},
        {16 * k,  L"8.0"},
        {17 * k,  L"8.5"},
        {18 * k,  L"9.0"},
        {19 * k,  L"9.5"},
        {20 * k, L"10.0"},
      };
  }

  return {};
}

////////////////////////////////////////////////////////////////////////////////

anime::AgeRating TranslateAgeRatingFrom(const std::string& value) {
  static const std::map<std::string, anime::AgeRating> table{
    {"G", anime::AgeRating::G},
    {"PG", anime::AgeRating::PG},
    {"R", anime::AgeRating::R17},
    {"R18", anime::AgeRating::R18},
  };

  const auto it = table.find(value);
  if (it != table.end())
    return it->second;

  if (!value.empty())
    LOGD(L"Invalid value: {}", StrToWstr(value));

  return anime::AgeRating::Unknown;
}

double TranslateSeriesRatingFrom(const std::string& value) {
  return ToDouble(value) / 10.0;
}

double TranslateSeriesRatingTo(double value) {
  return value * 10.0;
}

anime::SeriesStatus TranslateSeriesStatusFrom(const std::string& value) {
  static const std::map<std::string, anime::SeriesStatus> table{
    {"current", anime::SeriesStatus::Airing},
    {"finished", anime::SeriesStatus::FinishedAiring},
    {"tba", anime::SeriesStatus::NotYetAired},
    {"unreleased", anime::SeriesStatus::NotYetAired},
    {"upcoming", anime::SeriesStatus::NotYetAired},
  };

  const auto it = table.find(value);
  if (it != table.end())
    return it->second;

  if (!value.empty())
    LOGW(L"Invalid value: {}", StrToWstr(value));

  return anime::SeriesStatus::Unknown;
}

anime::SeriesType TranslateSeriesTypeFrom(const std::string& value) {
  static const std::map<std::string, anime::SeriesType> table{
    {"TV", anime::SeriesType::Tv},
    {"special", anime::SeriesType::Special},
    {"OVA", anime::SeriesType::Ova},
    {"ONA", anime::SeriesType::Ona},
    {"movie", anime::SeriesType::Movie},
    {"music", anime::SeriesType::Music},
  };

  const auto it = table.find(value);
  if (it != table.end())
    return it->second;

  if (!value.empty())
    LOGW(L"Invalid value: {}", StrToWstr(value));

  return anime::SeriesType::Unknown;
}

std::wstring TranslateMyDateFrom(const std::string& value) {
  return value.size() >= 10 ? StrToWstr(value.substr(0, 10)) : std::wstring();
}

std::string TranslateMyDateTo(const std::wstring& value) {
  return WstrToStr(value) + "T00:00:00.000Z";
}

std::wstring TranslateMyLastUpdatedFrom(const std::string& value) {
  // Get Unix time from ISO 8601
  const auto result = ConvertIso8601(StrToWstr(value));
  return result != -1 ? ToWstr(result) : std::wstring();
}

std::wstring TranslateMyRating(int value, RatingSystem rating_system) {
  value = (value * 20) / anime::kUserScoreMax;

  switch (rating_system) {
    case RatingSystem::Simple:
      switch (static_cast<int>(std::ceil(value / 5.0))) {
        case 0: return L"-";
        case 1: return L"Awful";
        case 2: return L"Meh";
        case 3: return L"Good";
        case 4: return L"Great";
      }
      break;

    case RatingSystem::Regular:
      value = value / 2;
      return L"\u2605 " + ToWstr(value / 2.0, 1);

    case RatingSystem::Advanced:
      if (value == 1)
        break;  // there is no "0.5" rating
      return ToWstr(value / 2.0, 1);
  }

  LOGW(L"Invalid value: {}", value);
  return ToWstr(value);
}

int TranslateMyRatingFrom(int value) {
  return value * (anime::kUserScoreMax / 20);
}

int TranslateMyRatingTo(int value) {
  return (value * 20) / anime::kUserScoreMax;
}

anime::MyStatus TranslateMyStatusFrom(const std::string& value) {
  static const std::map<std::string, anime::MyStatus> table{
    {"current", anime::MyStatus::Watching},
    {"planned", anime::MyStatus::PlanToWatch},
    {"completed", anime::MyStatus::Completed},
    {"on_hold", anime::MyStatus::OnHold},
    {"dropped", anime::MyStatus::Dropped},
  };

  const auto it = table.find(value);
  if (it != table.end())
    return it->second;

  LOGW(L"Invalid value: {}", StrToWstr(value));
  return anime::MyStatus::NotInList;
}

std::string TranslateMyStatusTo(anime::MyStatus value) {
  switch (value) {
    case anime::MyStatus::Watching: return "current";
    case anime::MyStatus::Completed: return "completed";
    case anime::MyStatus::OnHold: return "on_hold";
    case anime::MyStatus::Dropped: return "dropped";
    case anime::MyStatus::PlanToWatch: return "planned";
  }

  LOGW(L"Invalid value: {}", static_cast<int>(value));
  return "";
}

RatingSystem TranslateRatingSystemFrom(const std::string& value) {
  static const std::map<std::string, RatingSystem> table{
    {"simple", RatingSystem::Simple},
    {"regular", RatingSystem::Regular},
    {"advanced", RatingSystem::Advanced},
  };

  const auto it = table.find(value);
  if (it != table.end())
    return it->second;

  if (!value.empty())  // Can be empty for logged out users
    LOGD(L"Invalid value: {}", StrToWstr(value));

  return kDefaultRatingSystem;
}

////////////////////////////////////////////////////////////////////////////////

std::wstring GetAnimePage(const anime::Item& anime_item) {
  return L"https://kitsu.app/anime/{}"_format(anime_item.GetSlug());
}

void ViewAnimePage(int anime_id) {
  if (const auto anime_item = anime::db.Find(anime_id))
    ExecuteLink(GetAnimePage(*anime_item));
}

void ViewFeed() {
  ExecuteLink(L"https://kitsu.app");
}

void ViewLibrary() {
  ExecuteLink(L"https://kitsu.app/users/{}/library"_format(
      taiga::settings.GetSyncServiceKitsuUsername()));
}

void ViewProfile() {
  ExecuteLink(L"https://kitsu.app/users/{}"_format(
      taiga::settings.GetSyncServiceKitsuUsername()));
}

}  // namespace sync::kitsu
