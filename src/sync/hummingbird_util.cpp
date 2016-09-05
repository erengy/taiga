/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#include "base/file.h"
#include "base/log.h"
#include "base/string.h"
#include "base/time.h"
#include "library/anime.h"
#include "library/anime_db.h"
#include "sync/hummingbird_util.h"
#include "sync/hummingbird_types.h"
#include "taiga/settings.h"

namespace sync {
namespace hummingbird {

int TranslateAgeRatingFrom(const std::wstring& value) {
  if (value.empty()) {
    return anime::kUnknownAgeRating;
  } else if (IsEqual(value, L"G")) {
    return anime::kAgeRatingG;
  } else if (IsEqual(value, L"PG")) {
    return anime::kAgeRatingPG;
  } else if (IsEqual(value, L"PG13")) {
    return anime::kAgeRatingPG13;
  } else if (IsEqual(value, L"R17+")) {
    return anime::kAgeRatingR17;
  } else if (IsEqual(value, L"R18+")) {
    return anime::kAgeRatingR18;
  }

  LOG(LevelDebug, L"Invalid value: " + value);
  return anime::kUnknownAgeRating;
}

double TranslateSeriesRatingFrom(float value) {
  return static_cast<double>(value) * 2.0;
}

double TranslateSeriesRatingTo(double value) {
  return value / 2.0;
}

int TranslateSeriesStatusFrom(int value) {
  switch (value) {
    case kUnknownStatus: return anime::kUnknownStatus;
    case kCurrentlyAiring: return anime::kAiring;
    case kFinishedAiring: return anime::kFinishedAiring;
    case kNotYetAired: return anime::kNotYetAired;
  }

  LOG(LevelWarning, L"Invalid value: " + ToWstr(value));
  return anime::kUnknownStatus;
}

int TranslateSeriesStatusFrom(const std::wstring& value) {
  if (IsEqual(value, L"Currently Airing")) {
    return anime::kAiring;
  } else if (IsEqual(value, L"Finished Airing")) {
    return anime::kFinishedAiring;
  } else if (IsEqual(value, L"Not Yet Aired")) {
    return anime::kNotYetAired;
  }

  LOG(LevelWarning, L"Invalid value: " + value);
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

  LOG(LevelWarning, L"Invalid value: " + ToWstr(value));
  return anime::kUnknownType;
}

int TranslateSeriesTypeFrom(const std::wstring& value) {
  if (IsEqual(value, L"TV")) {
    return anime::kTv;
  } else if (IsEqual(value, L"Movie")) {
    return anime::kMovie;
  } else if (IsEqual(value, L"OVA")) {
    return anime::kOva;
  } else if (IsEqual(value, L"ONA")) {
    return anime::kOna;
  } else if (IsEqual(value, L"Special")) {
    return anime::kSpecial;
  } else if (IsEqual(value, L"Music")) {
    return anime::kMusic;
  }

  LOG(LevelWarning, L"Invalid value: " + value);
  return anime::kUnknownType;
}

std::wstring TranslateDateFrom(const std::wstring& value) {
  if (value.size() < 10)
    return Date();

  // Get YYYY-MM-DD from YYYY-MM-DDTHH:MM:SS.000Z (ISO 8601)
  return value.substr(0, 10);
}

std::wstring TranslateMyLastUpdatedFrom(const std::wstring& value) {
  // Get Unix time from ISO 8601
  auto result = ConvertIso8601(value);
  return result != -1 ? ToWstr(result) : std::wstring();
}

int TranslateMyStatusFrom(const std::wstring& value) {
  if (IsEqual(value, L"currently-watching")) {
    return anime::kWatching;
  } else if (IsEqual(value, L"plan-to-watch")) {
    return anime::kPlanToWatch;
  } else if (IsEqual(value, L"completed")) {
    return anime::kCompleted;
  } else if (IsEqual(value, L"on-hold")) {
    return anime::kOnHold;
  } else if (IsEqual(value, L"dropped")) {
    return anime::kDropped;
  }

  LOG(LevelWarning, L"Invalid value: " + value);
  return anime::kNotInList;
}

int TranslateMyRatingFrom(const std::wstring& value, const std::wstring& type) {
  return static_cast<int>(ToDouble(value) * 2.0);
}

std::wstring TranslateMyRatingTo(int value) {
  return ToWstr(static_cast<double>(value) / 2.0, 1);
}

std::wstring TranslateMyStatusTo(int value) {
  switch (value) {
    case anime::kWatching: return L"currently-watching";
    case anime::kCompleted: return L"completed";
    case anime::kOnHold: return L"on-hold";
    case anime::kDropped: return L"dropped";
    case anime::kPlanToWatch: return L"plan-to-watch";
  }

  LOG(LevelWarning, L"Invalid value: " + ToWstr(value));
  return L"";
}

////////////////////////////////////////////////////////////////////////////////

std::wstring GetAnimePage(const anime::Item& anime_item) {
  return L"https://hummingbird.me/anime/" + anime_item.GetSlug();
}

void ViewAnimePage(int anime_id) {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (anime_item)
    ExecuteLink(GetAnimePage(*anime_item));
}

void ViewDashboard() {
  ExecuteLink(L"https://hummingbird.me/dashboard");
}

void ViewProfile() {
  ExecuteLink(L"https://hummingbird.me/users/" +
              Settings[taiga::kSync_Service_Hummingbird_Username]);
}

void ViewRecommendations() {
  ExecuteLink(L"https://hummingbird.me/recommendations");
}

void ViewUpcomingAnime() {
  ExecuteLink(L"https://hummingbird.me/anime/upcoming");
}

}  // namespace hummingbird
}  // namespace sync