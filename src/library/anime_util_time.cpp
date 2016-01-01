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

#include <map>

#include "base/string.h"
#include "base/time.h"
#include "library/anime_season.h"
#include "library/anime_util.h"

namespace anime {

bool IsValidDate(const Date& date) {
  return date.year > 0;
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

std::wstring TranslateDateToSeasonString(const Date& date) {
  if (!IsValidDate(date))
    return L"Unknown";

  return Season(date).GetString();
}

std::wstring TranslateSeasonToMonths(const Season& season) {
  const wchar_t* months[] = {
    L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
    L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
  };

  Date date_start, date_end;
  season.GetInterval(date_start, date_end);

  std::wstring result = months[date_start.month - 1];
  result += L" " + ToWstr(date_start.year) + L" to ";
  result += months[date_end.month - 1];
  result += L" " + ToWstr(date_end.year);

  return result;
}

}  // namespace anime