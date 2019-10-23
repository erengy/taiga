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
#include <chrono>
#include <regex>

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
#include "sync/myanimelist_util.h"
#include "sync/service.h"
#include "taiga/http.h"
#include "taiga/settings.h"

namespace sync {
namespace myanimelist {

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

int TranslateAgeRatingFrom(const std::wstring& value) {
  static const std::map<std::wstring, anime::AgeRating> table{
    {L"g", anime::kAgeRatingG},
    {L"pg", anime::kAgeRatingPG},
    {L"pg_13", anime::kAgeRatingPG13},
    {L"r", anime::kAgeRatingR17},
    {L"r+", anime::kAgeRatingR17},
    {L"rx", anime::kAgeRatingR18},
  };

  const auto it = table.find(value);
  if (it != table.end())
    return it->second;

  if (!value.empty())
    LOGD(L"Invalid value: {}", value);

  return anime::kUnknownAgeRating;
}

int TranslateEpisodeLengthFrom(int value) {
  const auto seconds = std::chrono::seconds{value};
  return std::chrono::duration_cast<std::chrono::minutes>(seconds).count();
}

int TranslateSeriesStatusFrom(const std::wstring& value) {
  if (IsEqual(value, L"currently_airing")) {
    return anime::kAiring;
  } else if (IsEqual(value, L"finished_airing")) {
    return anime::kFinishedAiring;
  } else if (IsEqual(value, L"not_yet_aired")) {
    return anime::kNotYetAired;
  }

  LOGW(L"Invalid value: {}", value);
  return anime::kUnknownStatus;
}

int TranslateSeriesTypeFrom(const std::wstring& value) {
  if (IsEqual(value, L"unknown")) {
    return anime::kUnknownType;
  } else if (IsEqual(value, L"tv")) {
    return anime::kTv;
  } else if (IsEqual(value, L"ova")) {
    return anime::kOva;
  } else if (IsEqual(value, L"movie")) {
    return anime::kMovie;
  } else if (IsEqual(value, L"special")) {
    return anime::kSpecial;
  } else if (IsEqual(value, L"ona")) {
    return anime::kOna;
  } else if (IsEqual(value, L"music")) {
    return anime::kMusic;
  }

  if (!value.empty())
    LOGW(L"Invalid value: {}", value);

  return anime::kUnknownType;
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

int TranslateMyStatusFrom(const std::wstring& value) {
  if (IsEqual(value, L"watching")) {
    return anime::kWatching;
  } else if (IsEqual(value, L"completed")) {
    return anime::kCompleted;
  } else if (IsEqual(value, L"on_hold")) {
    return anime::kOnHold;
  } else if (IsEqual(value, L"dropped")) {
    return anime::kDropped;
  } else if (IsEqual(value, L"plan_to_watch")) {
    return anime::kPlanToWatch;
  }

  LOGW(L"Invalid value: {}", value);
  return anime::kNotInList;
}

std::wstring TranslateMyStatusTo(int value) {
  switch (value) {
    case anime::kWatching: return L"watching";
    case anime::kCompleted: return L"completed";
    case anime::kOnHold: return L"on_hold";
    case anime::kDropped: return L"dropped";
    case anime::kPlanToWatch: return L"plan_to_watch";
  }

  LOGW(L"Invalid value: {}", value);
  return L"watching";
}

std::wstring TranslateKeyTo(const std::wstring& key) {
  if (IsEqual(key, L"episode")) {
    return L"num_watched_episodes";
  } else if (IsEqual(key, L"status")) {
    return key;
  } else if (IsEqual(key, L"score")) {
    return key;
  } else if (IsEqual(key, L"date_start")) {
    return L"start_date";
  } else if (IsEqual(key, L"date_finish")) {
    return L"finish_date";
  } else if (IsEqual(key, L"enable_rewatching")) {
    return L"is_rewatching";
  } else if (IsEqual(key, L"rewatched_times")) {
    return L"num_times_rewatched";
  } else if (IsEqual(key, L"tags")) {
    return key;
  } else if (IsEqual(key, L"notes")) {
    return L"comments";
  }

  return std::wstring();
}

////////////////////////////////////////////////////////////////////////////////

std::wstring GetAnimePage(const anime::Item& anime_item) {
  return L"https://myanimelist.net/anime/" +
         anime_item.GetId(sync::kMyAnimeList) + L"/";
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
          kClientId, kRedirectUrl, code_verifier));
}

void RequestAccessToken(const std::wstring& authorization_code,
                        const std::wstring& code_verifier) {
  HttpRequest http_request;
  http_request.method = L"POST";
  http_request.url.protocol = base::http::Protocol::Https;
  http_request.url.host = L"myanimelist.net";
  http_request.url.path = L"/v1/oauth2/token";

  http_request.header = {{
      {L"Content-Type", L"application/x-www-form-urlencoded"},
    }};

  http_request.data = {{
      {L"client_id", kClientId},
      {L"grant_type", L"authorization_code"},
      {L"code", authorization_code},
      {L"redirect_uri", kRedirectUrl},
      {L"code_verifier", code_verifier},
    }};

  ConnectionManager.MakeRequest(http_request,
                                taiga::kHttpMalRequestAccessToken);
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

void ViewLogin() {
  ExecuteLink(L"https://myanimelist.net/login.php");
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

}  // namespace myanimelist
}  // namespace sync
