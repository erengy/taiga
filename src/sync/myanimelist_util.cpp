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

#include <algorithm>
#include <chrono>
#include <regex>

#include "sync/myanimelist_util.h"

#include "base/file.h"
#include "base/format.h"
#include "base/html.h"
#include "base/log.h"
#include "base/random.h"
#include "base/string.h"
#include "base/time.h"
#include "media/anime.h"
#include "media/anime_db.h"
#include "sync/myanimelist.h"
#include "sync/service.h"
#include "taiga/settings.h"

namespace sync::myanimelist {

std::vector<Rating> GetMyRatings() {
  constexpr int k = anime::kUserScoreMax / 10;

  return {
    { 0,      L"(0) No Score"},
    { 1 * k,  L"(1) Appalling"},
    { 2 * k,  L"(2) Horrible"},
    { 3 * k,  L"(3) Very Bad"},
    { 4 * k,  L"(4) Bad"},
    { 5 * k,  L"(5) Average"},
    { 6 * k,  L"(6) Fine"},
    { 7 * k,  L"(7) Good"},
    { 8 * k,  L"(8) Very Good"},
    { 9 * k,  L"(9) Great"},
    {10 * k, L"(10) Masterpiece"},
  };
}

////////////////////////////////////////////////////////////////////////////////

anime::AgeRating TranslateAgeRatingFrom(const std::wstring& value) {
  static const std::map<std::wstring, anime::AgeRating> table{
    {L"g", anime::AgeRating::G},
    {L"pg", anime::AgeRating::PG},
    {L"pg_13", anime::AgeRating::PG13},
    {L"r", anime::AgeRating::R17},
    {L"r+", anime::AgeRating::R17},
    {L"rx", anime::AgeRating::R18},
  };

  const auto it = table.find(ToLower_Copy(value));
  if (it != table.end())
    return it->second;

  if (!value.empty())
    LOGD(L"Invalid value: {}", value);

  return anime::AgeRating::Unknown;
}

Date TranslateDateFrom(const std::wstring& value) {
  // YYYY-MM-DD
  if (value.size() >= 10) {
    return Date(value);
  // YYYY-MM
  } else if (value.size() == 7) {
    return Date(L"{}-00"_format(value));
  // YYYY
  } else if (value.size() == 4) {
    return Date(L"{}-00-00"_format(value));
  } else {
    return Date{};
  }
}

int TranslateEpisodeLengthFrom(int value) {
  const auto seconds = std::chrono::seconds{value};
  return std::chrono::duration_cast<std::chrono::minutes>(seconds).count();
}

anime::SeriesStatus TranslateSeriesStatusFrom(const std::wstring& value) {
  static const std::map<std::wstring, anime::SeriesStatus> table{
    {L"currently_airing", anime::SeriesStatus::Airing},
    {L"finished_airing", anime::SeriesStatus::FinishedAiring},
    {L"not_yet_aired", anime::SeriesStatus::NotYetAired},
  };

  const auto it = table.find(ToLower_Copy(value));
  if (it != table.end())
    return it->second;

  LOGW(L"Invalid value: {}", value);
  return anime::SeriesStatus::Unknown;
}

anime::SeriesType TranslateSeriesTypeFrom(const std::wstring& value) {
  static const std::map<std::wstring, anime::SeriesType> table{
    {L"unknown", anime::SeriesType::Unknown},
    {L"tv", anime::SeriesType::Tv},
    {L"ova", anime::SeriesType::Ova},
    {L"movie", anime::SeriesType::Movie},
    {L"special", anime::SeriesType::Special},
    {L"ona", anime::SeriesType::Ona},
    {L"music", anime::SeriesType::Music},
    {L"cm", anime::SeriesType::Special},
    {L"pv", anime::SeriesType::Special},
    {L"tv_special", anime::SeriesType::Special},
  };

  const auto it = table.find(ToLower_Copy(value));
  if (it != table.end())
    return it->second;

  if (!value.empty())
    LOGW(L"Invalid value: {}", value);

  return anime::SeriesType::Unknown;
}

std::wstring TranslateMyLastUpdatedFrom(const std::string& value) {
  // Get Unix time from ISO 8601
  const auto result = ConvertIso8601(StrToWstr(value));
  return result != -1 ? ToWstr(result) : std::wstring();
}

std::wstring TranslateMyRating(int value, bool full) {
  if (!full)
    return ToWstr(TranslateMyRatingTo(value));

  switch (TranslateMyRatingTo(value)) {
    case  0: return  L"(0) No Score";
    case  1: return  L"(1) Appalling";
    case  2: return  L"(2) Horrible";
    case  3: return  L"(3) Very Bad";
    case  4: return  L"(4) Bad";
    case  5: return  L"(5) Average";
    case  6: return  L"(6) Fine";
    case  7: return  L"(7) Good";
    case  8: return  L"(8) Very Good";
    case  9: return  L"(9) Great";
    case 10: return L"(10) Masterpiece";
  }

  LOGW(L"Invalid value: {}", value);
  return ToWstr(value);
}

int TranslateMyRatingFrom(int value) {
  return value * (anime::kUserScoreMax / 10);
}

int TranslateMyRatingTo(int value) {
  return (value * 10) / anime::kUserScoreMax;
}

anime::MyStatus TranslateMyStatusFrom(const std::wstring& value) {
  static const std::map<std::wstring, anime::MyStatus> table{
    {L"watching", anime::MyStatus::Watching},
    {L"completed", anime::MyStatus::Completed},
    {L"on_hold", anime::MyStatus::OnHold},
    {L"dropped", anime::MyStatus::Dropped},
    {L"plan_to_watch", anime::MyStatus::PlanToWatch},
  };

  const auto it = table.find(ToLower_Copy(value));
  if (it != table.end())
    return it->second;

  LOGW(L"Invalid value: {}", value);
  return anime::MyStatus::NotInList;
}

std::string TranslateMyStatusTo(anime::MyStatus value) {
  switch (value) {
    case anime::MyStatus::Watching: return "watching";
    case anime::MyStatus::Completed: return "completed";
    case anime::MyStatus::OnHold: return "on_hold";
    case anime::MyStatus::Dropped: return "dropped";
    case anime::MyStatus::PlanToWatch: return "plan_to_watch";
  }

  LOGW(L"Invalid value: {}", static_cast<int>(value));
  return "watching";
}

////////////////////////////////////////////////////////////////////////////////

std::wstring GetAnimePage(const anime::Item& anime_item) {
  return L"https://myanimelist.net/anime/" +
         anime_item.GetId(ServiceId::MyAnimeList) + L"/";
}

void RequestAuthorizationCode(std::wstring& code_verifier) {
  code_verifier.assign(64, '\0');
  std::generate(code_verifier.begin(), code_verifier.end(), []() {
    static const std::wstring unreserved =
        L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        L"abcdefghijklmnopqrstvuwxyz"
        L"0123456789-._~";
    return *Random::get(unreserved.begin(), unreserved.end());
  });

  ExecuteLink(L"https://myanimelist.net/v1/oauth2/authorize"
      L"?response_type=code"
      L"&client_id={}"
      L"&redirect_uri={}"
      L"&code_challenge={}"
      L"&code_challenge_method=plain"_format(
          StrToWstr(kClientId), StrToWstr(kRedirectUrl), code_verifier));
}

void ViewAnimePage(int anime_id) {
  auto anime_item = anime::db.Find(anime_id);

  if (anime_item)
    ExecuteLink(GetAnimePage(*anime_item));
}

void ViewAnimeSearch(const std::wstring& title) {
  ExecuteLink(L"https://myanimelist.net/anime.php?q=" + title);
}

void ViewHistory() {
  ExecuteLink(L"https://myanimelist.net/history/" +
              taiga::settings.GetSyncServiceMalUsername());
}

void ViewPanel() {
  ExecuteLink(L"https://myanimelist.net/panel.php");
}

void ViewProfile() {
  ExecuteLink(L"https://myanimelist.net/profile/" +
              taiga::settings.GetSyncServiceMalUsername());
}

void ViewUpcomingAnime() {
  Date date = GetDate();

  ExecuteLink(L"https://myanimelist.net/anime.php"
              L"?sd=" + ToWstr(date.day()) +
              L"&sm=" + ToWstr(date.month()) +
              L"&sy=" + ToWstr(date.year()) +
              L"&em=0&ed=0&ey=0&o=2&w=&c[]=a&c[]=d&cv=1");
}

}  // namespace sync::myanimelist
