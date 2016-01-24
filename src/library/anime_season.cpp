/*
** Taiga
** Copyright (C) 2010-2016, Eren Okka
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
#include <regex>
#include <vector>

#include "base/string.h"
#include "base/time.h"
#include "library/anime_season.h"

namespace anime {

Season::Season()
    : name(kUnknown), year(0) {
}

Season::Season(Name name, unsigned short year)
    : name(name), year(year) {
}

Season::Season(const Date& date) {
  year = date.year;

  if (date.month == 0 || date.month > 12) {
    name = kUnknown;
  } else if (date.month < 3) {   // Jan-Feb
    name = kWinter;
  } else if (date.month < 6) {   // Mar-May
    name = kSpring;
  } else if (date.month < 9) {   // Jun-Aug
    name = kSummer;
  } else if (date.month < 12) {  // Sep-Nov
    name = kFall;
  } else {                       // Dec
    name = kWinter;
    year += 1;
  }
}

Season::Season(const std::wstring& str) : Season() {
  static const std::map<std::wstring, Season::Name> seasons{
    {L"winter", kWinter},
    {L"spring", kSpring},
    {L"summer", kSummer},
    {L"fall", kFall},
  };

  static const std::wregex pattern(L"([A-Za-z]+)[ _](\\d{4})");
  std::wsmatch match_results;

  if (std::regex_match(str, match_results, pattern)) {
    auto it = seasons.find(ToLower_Copy(match_results[1]));
    if (it != seasons.end())
      name = it->second;
    year = ToInt(match_results[2]);
  }
}

Season& Season::operator=(const Season& season) {
  name = season.name;
  year = season.year;

  return *this;
}

Season& Season::operator++() {
  switch (name) {
    case kWinter:
      name = kSpring;
      break;
    case kSpring:
      name = kSummer;
      break;
    case kSummer:
      name = kFall;
      break;
    case kFall:
      name = kWinter;
      ++year;
      break;
  }

  return *this;
}

Season& Season::operator--() {
  switch (name) {
    case kWinter:
      name = kFall;
      --year;
      break;
    case kSpring:
      name = kWinter;
      break;
    case kSummer:
      name = kSpring;
      break;
    case kFall:
      name = kSummer;
      break;
  }

  return *this;
}

Season::operator bool() const {
  return name != kUnknown && year > 0;
}

////////////////////////////////////////////////////////////////////////////////

void Season::GetInterval(Date& date_start, Date& date_end) const {
  static std::map<Name, std::pair<int, int>> interval{
    {kUnknown, {0, 0}},
    {kWinter, {12, 2}},
    {kSpring, {3, 5}},
    {kSummer, {6, 8}},
    {kFall, {9, 11}},
  };
  static const std::vector<int> days_in_months{
    31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
  };

  date_start.year = name == kWinter ? year - 1 : year;
  date_start.month = interval[name].first;
  date_start.day = 1;

  date_end.year = year;
  date_end.month = interval[name].second;
  date_end.day = days_in_months[date_end.month - 1];
}

std::wstring Season::GetName() const {
  static std::map<Season::Name, std::wstring> seasons{
    {kUnknown, L"Unknown"},
    {kWinter, L"Winter"},
    {kSpring, L"Spring"},
    {kSummer, L"Summer"},
    {kFall, L"Fall"},
  };

  return seasons[name];
}

std::wstring Season::GetString() const {
  static std::map<Season::Name, std::wstring> seasons{
    {kUnknown, L""},
    {kWinter, L"Winter "},
    {kSpring, L"Spring "},
    {kSummer, L"Summer "},
    {kFall, L"Fall "},
  };

  return seasons[name] + ToWstr(year);
}

////////////////////////////////////////////////////////////////////////////////

base::CompareResult Season::Compare(const Season& season) const {
  if (year != season.year) {
    if (year == 0)
      return base::kGreaterThan;
    if (season.year == 0)
      return base::kLessThan;
    return year < season.year ? base::kLessThan : base::kGreaterThan;
  }

  if (name != season.name) {
    if (name == Season::kUnknown)
      return base::kGreaterThan;
    if (season.name == Season::kUnknown)
      return base::kLessThan;
    return name < season.name ? base::kLessThan : base::kGreaterThan;
  }

  return base::kEqualTo;
}

}  // namespace anime