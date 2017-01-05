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

#include <regex>

#include "base/file.h"
#include "base/html.h"
#include "base/log.h"
#include "base/string.h"
#include "base/time.h"
#include "library/anime.h"
#include "library/anime_db.h"
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

  return text;
}

std::wstring EraseBbcode(std::wstring& str) {
  using namespace std::regex_constants;
  const std::wregex pattern(L"\\[/?(b|i|u|(size(=[0-9]+)?)|(url(=[^\\]]*)?))\\]",
                            nosubs | optimize);
  return std::regex_replace(str, pattern, L"");
}

////////////////////////////////////////////////////////////////////////////////

int TranslateSeriesStatusFrom(int value) {
  switch (value) {
    case kUnknownStatus: return anime::kUnknownStatus;
    case kAiring: return anime::kAiring;
    case kFinishedAiring: return anime::kFinishedAiring;
    case kNotYetAired: return anime::kNotYetAired;
  }

  LOGW(L"Invalid value: " + ToWstr(value));
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

  LOGW(L"Invalid value: " + value);
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

  LOGW(L"Invalid value: " + ToWstr(value));
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

  LOGW(L"Invalid value: " + value);
  return anime::kUnknownType;
}

std::wstring TranslateMyDateTo(const std::wstring& value) {
  Date date(value);

  // Convert YYYY-MM-DD to MMDDYYYY
  return PadChar(ToWstr(date.month()), '0', 2) +
         PadChar(ToWstr(date.day()), '0', 2) +
         PadChar(ToWstr(date.year()), '0', 4);
}

int TranslateMyStatusFrom(int value) {
  switch (value) {
    case kWatching: return anime::kWatching;
    case kCompleted: return anime::kCompleted;
    case kOnHold: return anime::kOnHold;
    case kDropped: return anime::kDropped;
    case kPlanToWatch: return anime::kPlanToWatch;
  }

  LOGW(L"Invalid value: " + ToWstr(value));
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

  LOGW(L"Invalid value: " + ToWstr(value));
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
  auto anime_item = AnimeDatabase.FindItem(anime_id);

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