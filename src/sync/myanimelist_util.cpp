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

#include <regex>

#include "base/file.h"
#include "base/html.h"
#include "base/log.h"
#include "base/string.h"
#include "base/time.h"
#include "media/anime.h"
#include "media/anime_db.h"
#include "sync/myanimelist_util.h"
#include "sync/myanimelist_types.h"
#include "sync/service.h"
#include "taiga/settings.h"

namespace sync {
namespace myanimelist {

std::wstring DecodeText(std::wstring text) {
  ReplaceString(text, L"<br />", L"\r");
  ReplaceString(text, L"\n\n", L"\r\n\r\n");

  StripHtmlTags(text);
  DecodeHtmlEntities(text);
  Trim(text, L" \t\r\n");

  return text;
}

std::wstring EraseBbcode(std::wstring& str) {
  using namespace std::regex_constants;
  const std::wregex pattern(
      L"\\[/?(b|i|quote|u|(color(=[#\\w]+)?)|(size(=[0-9]+)?)|(url(=[^\\]]*)?))\\]",
      nosubs | optimize);
  return std::regex_replace(str, pattern, L"");
}

////////////////////////////////////////////////////////////////////////////////

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

int TranslateSeriesStatusFrom(int value) {
  switch (value) {
    case kUnknownStatus: return anime::kUnknownStatus;
    case kAiring: return anime::kAiring;
    case kFinishedAiring: return anime::kFinishedAiring;
    case kNotYetAired: return anime::kNotYetAired;
  }

  LOGW(L"Invalid value: {}", value);
  return anime::kUnknownStatus;
}

int TranslateSeriesStatusFrom(const std::wstring& value) {
  if (IsEqual(value, L"Currently airing")) {
    return anime::kAiring;
  } else if (IsEqual(value, L"Finished airing")) {
    return anime::kFinishedAiring;
  } else if (IsEqual(value, L"Not yet aired")) {
    return anime::kNotYetAired;
  }

  LOGW(L"Invalid value: {}", value);
  return anime::kUnknownStatus;
}

int TranslateSeriesTypeFrom(int value) {
  switch (value) {
    case kUnknownType: return anime::kUnknownType;
    case kTv: return anime::kTv;
    case kOva: return anime::kOva;
    case kMovie: return anime::kMovie;
    case kSpecial: return anime::kSpecial;
    case kOna: return anime::kOna;
    case kMusic: return anime::kMusic;
  }

  LOGW(L"Invalid value: {}", value);
  return anime::kUnknownType;
}

int TranslateSeriesTypeTo(int value) {
  switch (value) {
    case anime::kUnknownType: return kUnknownType;
    case anime::kTv: return kTv;
    case anime::kOva: return kOva;
    case anime::kMovie: return kMovie;
    case anime::kSpecial: return kSpecial;
    case anime::kOna: return kOna;
    case anime::kMusic: return kMusic;
  }

  LOGW(L"Invalid value: {}", value);
  return anime::kUnknownType;
}

int TranslateSeriesTypeFrom(const std::wstring& value) {
  if (IsEqual(value, L"TV")) {
    return anime::kTv;
  } else if (IsEqual(value, L"OVA")) {
    return anime::kOva;
  } else if (IsEqual(value, L"Movie")) {
    return anime::kMovie;
  } else if (IsEqual(value, L"Special")) {
    return anime::kSpecial;
  } else if (IsEqual(value, L"ONA")) {
    return anime::kOna;
  } else if (IsEqual(value, L"Music")) {
    return anime::kMusic;
  }

  if (!value.empty())
    LOGW(L"Invalid value: {}", value);

  return anime::kUnknownType;
}

std::wstring TranslateMyDateTo(const std::wstring& value) {
  Date date(value);

  // Convert YYYY-MM-DD to MMDDYYYY
  return PadChar(ToWstr(date.month()), '0', 2) +
         PadChar(ToWstr(date.day()), '0', 2) +
         PadChar(ToWstr(date.year()), '0', 4);
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

int TranslateMyStatusFrom(int value) {
  switch (value) {
    case kWatching: return anime::kWatching;
    case kCompleted: return anime::kCompleted;
    case kOnHold: return anime::kOnHold;
    case kDropped: return anime::kDropped;
    case kPlanToWatch: return anime::kPlanToWatch;
  }

  LOGW(L"Invalid value: {}", value);
  return anime::kNotInList;
}

int TranslateMyStatusTo(int value) {
  switch (value) {
    case anime::kWatching: return kWatching;
    case anime::kCompleted: return kCompleted;
    case anime::kOnHold: return kOnHold;
    case anime::kDropped: return kDropped;
    case anime::kPlanToWatch: return kPlanToWatch;
  }

  LOGW(L"Invalid value: {}", value);
  return kWatching;
}

std::wstring TranslateKeyTo(const std::wstring& key) {
  if (IsEqual(key, L"episode")) {
    return key;
  } else if (IsEqual(key, L"status")) {
    return key;
  } else if (IsEqual(key, L"score")) {
    return key;
  } else if (IsEqual(key, L"date_start")) {
    return key;
  } else if (IsEqual(key, L"date_finish")) {
    return key;
  } else if (IsEqual(key, L"enable_rewatching")) {
    return key;
  } else if (IsEqual(key, L"tags")) {
    return key;
  }

  return std::wstring();
}

////////////////////////////////////////////////////////////////////////////////

std::wstring GetAnimePage(const anime::Item& anime_item) {
  return L"https://myanimelist.net/anime/" +
         anime_item.GetId(sync::kMyAnimeList) + L"/";
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
              Settings[taiga::kSync_Service_Mal_Username]);
}

void ViewLogin() {
  ExecuteLink(L"https://myanimelist.net/login.php");
}

void ViewPanel() {
  ExecuteLink(L"https://myanimelist.net/panel.php");
}

void ViewProfile() {
  ExecuteLink(L"https://myanimelist.net/profile/" +
              Settings[taiga::kSync_Service_Mal_Username]);
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