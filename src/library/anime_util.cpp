/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

#include "anime.h"
#include "anime_db.h"
#include "anime_util.h"

#include "base/foreach.h"
#include "base/string.h"
#include "base/time.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "track/feed.h"

namespace anime {

bool GetFansubFilter(int anime_id, vector<wstring>& groups) {
  bool found = false;
  
  foreach_(i, Aggregator.filter_manager.filters) {
    if (found) break;
    foreach_(j, i->anime_ids) {
      if (*j != anime_id) continue;
      if (found) break;
      foreach_(k, i->conditions) {
        if (k->element == FEED_FILTER_ELEMENT_EPISODE_GROUP) {
          groups.push_back(k->value);
          found = true;
        }
      }
    }
  }

  return found;
}

bool SetFansubFilter(int anime_id, const wstring& group_name) {
  // Check existing filters
  foreach_(i, Aggregator.filter_manager.filters) {
    foreach_(j, i->anime_ids) {
      if (*j != anime_id) continue;
      foreach_(k, i->conditions) {
        if (k->element == FEED_FILTER_ELEMENT_EPISODE_GROUP) {
          if (group_name.empty()) {
            Aggregator.filter_manager.filters.erase(i);
          } else {
            k->value = group_name;
          }
          return true;
        }
      }
    }
  }

  if (group_name.empty())
    return false;

  // Create new filter
  auto anime_item = AnimeDatabase.FindItem(anime_id);
  Aggregator.filter_manager.AddFilter(
    FEED_FILTER_ACTION_PREFER, FEED_FILTER_MATCH_ALL, true,
    L"[Fansub] " + anime_item->GetTitle());
  Aggregator.filter_manager.filters.back().AddCondition(
    FEED_FILTER_ELEMENT_EPISODE_GROUP, FEED_FILTER_OPERATOR_EQUALS,
    group_name);
  Aggregator.filter_manager.filters.back().anime_ids.push_back(anime_id);
  return true;
}

wstring GetImagePath(int anime_id) {
  wstring path = taiga::GetPath(taiga::kPathDatabaseImage);
  if (anime_id > 0) path += ToWstr(anime_id) + L".jpg";
  return path;
}

void GetUpcomingTitles(vector<int>& anime_ids) {
  foreach_c_(item, AnimeDatabase.items) {
    const anime::Item& anime_item = item->second;
    
    const Date& date_start = anime_item.GetDateStart();
    const Date& date_now = GetDateJapan();

    if (!date_start.year || !date_start.month || !date_start.day)
      continue;

    if (date_start > date_now &&
        ToDayCount(date_start) < ToDayCount(date_now) + 7) { // Same week
      anime_ids.push_back(anime_item.GetId());
    }
  }
}

bool IsInsideRootFolders(const wstring& path) {
  foreach_c_(root_folder, Settings.Folders.root)
    if (StartsWith(path, *root_folder))
      return true;

  return false;
}

////////////////////////////////////////////////////////////////////////////////

bool IsValidDate(const Date& date) {
  return date.year > 0;
}

bool IsValidDate(const wstring& date) {
  return date.length() == 10 && !StartsWith(date, L"0000");
}

bool IsValidEpisode(int episode, int watched, int total) {
  if ((episode < 0) ||
      (episode < watched) ||
      (episode == watched && total != 1) ||
      (episode > total && total != 0))
    return false;

  return true;
}

Date ParseDateString(const wstring& str) {
  Date date;

  if (IsValidDate(str)) {
    date.year  = ToInt(str.substr(0, 4));
    date.month = ToInt(str.substr(5, 2));
    date.day   = ToInt(str.substr(8, 2));
  }

  return date;
}

void GetSeasonInterval(const wstring& season, Date& date_start, Date& date_end) {
  std::map<wstring, std::pair<int, int>> interval;
  interval[L"Spring"] = std::make_pair<int, int>(3, 5);
  interval[L"Summer"] = std::make_pair<int, int>(6, 8);
  interval[L"Fall"] = std::make_pair<int, int>(9, 11);
  interval[L"Winter"] = std::make_pair<int, int>(12, 2);

  const int days_in_months[] = 
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  vector<wstring> season_year;
  Split(season, L" ", season_year);

  date_start.year = ToInt(season_year.at(1));
  date_end.year = ToInt(season_year.at(1));
  if (season_year.at(0) == L"Winter") date_start.year--;
  date_start.month = interval[season_year.at(0)].first;
  date_end.month = interval[season_year.at(0)].second;
  date_start.day = 1;
  date_end.day = days_in_months[date_end.month - 1];
}

////////////////////////////////////////////////////////////////////////////////

int EstimateEpisodeCount(const Item& item) {
  // If we already know the number, we don't need to estimate
  if (item.GetEpisodeCount() > 0)
    return item.GetEpisodeCount();

  int number = 0;

  // Estimate using user information
  if (item.IsInList())
    number = max(item.GetMyLastWatchedEpisode(),
                 item.GetAvailableEpisodeCount());

  // Estimate using airing dates of TV series
  if (item.GetType() == kTv) {
    Date date_start = item.GetDateStart();
    if (IsValidDate(date_start)) {
      Date date_end = item.GetDateEnd();
      // Use current date in Japan if ending date is unknown
      if (!IsValidDate(date_end)) date_end = GetDateJapan();
      // Assuming the series is aired weekly
      number = max(number, (date_end - date_start) / 7);
    }
  }

  // Given all TV series aired since 2000, most them have their episodes
  // spanning one or two seasons. Following is a table of top ten values:
  //
  //   Episodes    Seasons    Percent
  //   ------------------------------
  //         12          1      23.6%
  //         13          1      20.2%
  //         26          2      15.4%
  //         24          2       6.4%
  //         25          2       5.0%
  //         52          4       4.4%
  //         51          4       3.1%
  //         11          1       2.6%
  //         50          4       2.3%
  //         39          3       1.4%
  //   ------------------------------
  //   Total:                   84.6%
  //
  // With that in mind, we can normalize our output at several points.
  if (number < 12) return 13;
  if (number < 24) return 26;
  if (number < 50) return 52;

  // This is a series that has aired for more than a year, which means we cannot
  // estimate for how long it is going to continue.
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

wstring TranslateDate(const Date& date) {
  if (!IsValidDate(date))
    return L"?";

  wstring result;

  if (date.month > 0 && date.month <= 12) {
    const wchar_t* months[] = {
      L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", 
      L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
    };
    result += months[date.month - 1];
    result += L" ";
  }
  if (date.day > 0)
    result += ToWstr(date.day) + L", ";
  result += ToWstr(date.year);

  return result;
}

wstring TranslateDateForApi(const Date& date) {
  if (!IsValidDate(date))
    return L"";

  return PadChar(ToWstr(date.month), '0', 2) + // MM
         PadChar(ToWstr(date.day),   '0', 2) + // DD
         PadChar(ToWstr(date.year),  '0', 4);  // YYYY
}
Date TranslateDateFromApi(const wstring& date) {
  if (date.size() != 8)
    return Date();

  return Date(ToInt(date.substr(4, 4)), 
              ToInt(date.substr(0, 2)), 
              ToInt(date.substr(2, 2)));
}

wstring TranslateDateToSeason(const Date& date) {
  if (!IsValidDate(date))
    return L"Unknown";

  wstring season;
  unsigned short year = date.year;

  if (date.month == 0) {
    season = L"Unknown";
  } else if (date.month < 3) {  // Jan-Feb
    season = L"Winter";
  } else if (date.month < 6) {  // Mar-May
    season = L"Spring";
  } else if (date.month < 9) {  // Jun-Aug
    season = L"Summer";
  } else if (date.month < 12) { // Sep-Nov
    season = L"Fall";
  } else {                      // Dec
    season = L"Winter";
    year++;
  }
    
  return season + L" " + ToWstr(year);
}

wstring TranslateSeasonToMonths(const wstring& season) {
  const wchar_t* months[] = {
    L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", 
    L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
  };

  Date date_start, date_end;
  GetSeasonInterval(season, date_start, date_end);

  wstring result = months[date_start.month - 1];
  result += L" " + ToWstr(date_start.year) + L" to ";
  result += months[date_end.month - 1];
  result += L" " + ToWstr(date_end.year);
  
  return result;
}

wstring TranslateMyStatus(int value, bool add_count) {
  #define ADD_COUNT() (add_count ? L" (" + ToWstr(AnimeDatabase.GetItemCount(value)) + L")" : L"")
  switch (value) {
    case kNotInList: return L"Not in list";
    case kWatching: return L"Currently watching" + ADD_COUNT();
    case kCompleted: return L"Completed" + ADD_COUNT();
    case kOnHold: return L"On hold" + ADD_COUNT();
    case kDropped: return L"Dropped" + ADD_COUNT();
    case kPlanToWatch: return L"Plan to watch" + ADD_COUNT();
    default: return L"";
  }
  #undef ADD_COUNT
}
int TranslateMyStatus(const wstring& value) {
  if (IsEqual(value, L"Currently watching")) {
    return kWatching;
  } else if (IsEqual(value, L"Completed")) {
    return kCompleted;
  } else if (IsEqual(value, L"On hold")) {
    return kOnHold;
  } else if (IsEqual(value, L"Dropped")) {
    return kDropped;
  } else if (IsEqual(value, L"Plan to watch")) {
    return kPlanToWatch;
  } else {
    return 0;
  }
}

wstring TranslateNumber(int value, const wstring& default_char) {
  return value > 0 ? ToWstr(value) : default_char;
}

wstring TranslateStatus(int value) {
  switch (value) {
    case kAiring: return L"Currently airing";
    case kFinishedAiring: return L"Finished airing";
    case kNotYetAired: return L"Not yet aired";
    default: return ToWstr(value);
  }
}
int TranslateStatus(const wstring& value) {
  if (IsEqual(value, L"Currently airing")) {
    return kAiring;
  } else if (IsEqual(value, L"Finished airing")) {
    return kFinishedAiring;
  } else if (IsEqual(value, L"Not yet aired")) {
    return kNotYetAired;
  } else {
    return 0;
  }
}

wstring TranslateType(int value) {
  switch (value) {
    case kTv: return L"TV";
    case kOva: return L"OVA";
    case kMovie: return L"Movie";
    case kSpecial: return L"Special";
    case kOna: return L"ONA";
    case kMusic: return L"Music";
    default: return L"";
  }
}
int TranslateType(const wstring& value) {
  if (IsEqual(value, L"TV")) {
    return kTv;
  } else if (IsEqual(value, L"OVA")) {
    return kOva;
  } else if (IsEqual(value, L"Movie")) {
    return kMovie;
  } else if (IsEqual(value, L"Special")) {
    return kSpecial;
  } else if (IsEqual(value, L"ONA")) {
    return kOna;
  } else if (IsEqual(value, L"Music")) {
    return kMusic;
  } else {
    return 0;
  }
}

}  // namespace anime