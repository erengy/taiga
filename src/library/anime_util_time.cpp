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

#include <map>

#include "base/string.h"
#include "base/time.h"
#include "library/anime_util.h"

namespace anime {

bool IsValidDate(const Date& date) {
  return date.year > 0;
}

void GetSeasonInterval(const std::wstring& season, Date& date_start,
                       Date& date_end) {
  std::map<std::wstring, std::pair<int, int>> interval;
  interval[L"Spring"] = std::make_pair<int, int>(3, 5);
  interval[L"Summer"] = std::make_pair<int, int>(6, 8);
  interval[L"Fall"] = std::make_pair<int, int>(9, 11);
  interval[L"Winter"] = std::make_pair<int, int>(12, 2);

  const int days_in_months[] = 
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  std::vector<std::wstring> season_year;
  Split(season, L" ", season_year);

  date_start.year = ToInt(season_year.at(1));
  date_start.month = interval[season_year.at(0)].first;
  date_start.day = 1;

  if (season_year.at(0) == L"Winter")
    date_start.year--;

  date_end.year = ToInt(season_year.at(1));
  date_end.month = interval[season_year.at(0)].second;
  date_end.day = days_in_months[date_end.month - 1];
}

std::wstring TranslateDate(const Date& date) {
  if (!IsValidDate(date))
    return L"?";

  std::wstring result;

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

std::wstring TranslateDateToSeason(const Date& date) {
  if (!IsValidDate(date))
    return L"Unknown";

  std::wstring season;
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

std::wstring TranslateSeasonToMonths(const std::wstring& season) {
  const wchar_t* months[] = {
    L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", 
    L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
  };

  Date date_start, date_end;
  GetSeasonInterval(season, date_start, date_end);

  std::wstring result = months[date_start.month - 1];
  result += L" " + ToWstr(date_start.year) + L" to ";
  result += months[date_end.month - 1];
  result += L" " + ToWstr(date_end.year);

  return result;
}

}  // namespace anime