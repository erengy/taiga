/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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
#include <unordered_map>

#include <nstd/string.hpp>

#include "media/anime_season.h"

#include "base/string.h"
#include "sync/service.h"

namespace anime {

namespace {

constexpr auto UnknownSeason = Season::Name::Unknown;
constexpr auto Winter = Season::Name::Winter;
constexpr auto Spring = Season::Name::Spring;
constexpr auto Summer = Season::Name::Summer;
constexpr auto Fall = Season::Name::Fall;

bool ShiftedSeasons() {
  switch (sync::GetCurrentServiceId()) {
    case sync::ServiceId::MyAnimeList:
      return false;  // Winter: January, February, March
    default:
      return true;  // Winter: December, January, February
  }
}

Season::Name GetSeasonName(const date::month& month) {
  if (ShiftedSeasons()) {
    switch (static_cast<unsigned>(month)) {
      default: return UnknownSeason;
      case 12: case  1: case  2: return Winter;
      case  3: case  4: case  5: return Spring;
      case  6: case  7: case  8: return Summer;
      case  9: case 10: case 11: return Fall;
    }
  } else {
    switch (static_cast<unsigned>(month)) {
      default: return UnknownSeason;
      case  1: case  2: case  3: return Winter;
      case  4: case  5: case  6: return Spring;
      case  7: case  8: case  9: return Summer;
      case 10: case 11: case 12: return Fall;
    }
  }
}

constexpr Season::Name GetNextSeasonName(const Season::Name name) {
  switch (name) {
    default: return UnknownSeason;
    case Winter: return Spring;
    case Spring: return Summer;
    case Summer: return Fall;
    case Fall:   return Winter;
  }
}

constexpr Season::Name GetPreviousSeasonName(const Season::Name name) {
  switch (name) {
    default: return UnknownSeason;
    case Winter: return Fall;
    case Spring: return Winter;
    case Summer: return Spring;
    case Fall:   return Summer;
  }
}

std::pair<date::month, date::month> GetSeasonRange(const Season::Name name) {
  if (ShiftedSeasons()) {
    switch (name) {
      default: return {};
      case Winter: return {date::December,  date::February};
      case Spring: return {date::March,     date::May};
      case Summer: return {date::June,      date::August};
      case Fall:   return {date::September, date::November};
    }
  } else {
    switch (name) {
      default: return {};
      case Winter: return {date::January, date::March};
      case Spring: return {date::April,   date::June};
      case Summer: return {date::July,    date::September};
      case Fall:   return {date::October, date::December};
    }
  }
}

}  // namespace

Season::Season(const Date& date) {
  if (date.month()) {
    name = GetSeasonName(date::month{date.month()});
  }

  if (date.year()) {
    year = date::year{date.year()};
    if (ShiftedSeasons() && date.month() &&
        date::month{date.month()} == date::December) {
      ++year;  // e.g. December 2018 -> Winter 2019
    }
  }
}

Season::Season(const std::string& str) {
  static const std::unordered_map<std::string, Season::Name> seasons{
    {"winter", Winter},
    {"spring", Spring},
    {"summer", Summer},
    {"fall", Fall}
  };

  static const std::regex pattern{"([A-Za-z]{4,6})[ _](\\d{4})"};
  std::smatch match_results;

  if (std::regex_match(str, match_results, pattern)) {
    const auto it = seasons.find(nstd::tolower_string(match_results[1]));
    if (it != seasons.end()) {
      name = it->second;
    }
    year = date::year{ToInt(match_results[2])};
  }
}

////////////////////////////////////////////////////////////////////////////////

Season::operator bool() const {
  return name != UnknownSeason && static_cast<int>(year);
}

Season& Season::operator++() {
  name = GetNextSeasonName(name);

  if (name == Winter) {
    ++year;
  }

  return *this;
}

Season& Season::operator--() {
  name = GetPreviousSeasonName(name);

  if (name == Fall) {
    --year;
  }

  return *this;
}

////////////////////////////////////////////////////////////////////////////////

int Season::compare(const Season& season) const {
  if (year != season.year) {
    if (!static_cast<int>(year))
      return nstd::cmp::greater;
    if (!static_cast<int>(season.year))
      return nstd::cmp::less;
    return year < season.year ? nstd::cmp::less : nstd::cmp::greater;
  }

  if (name != season.name) {
    if (name == UnknownSeason)
      return nstd::cmp::greater;  // Unknown seasons are in the future
    if (season.name == UnknownSeason)
      return nstd::cmp::less;
    return name < season.name ? nstd::cmp::less : nstd::cmp::greater;
  }

  return nstd::cmp::equal;
}

std::pair<DateFull, DateFull> Season::to_date_range() const {
  if (!(*this)) {
    return {};
  }

  const auto months = GetSeasonRange(name);

  return {
    {
      ShiftedSeasons() && name == Winter ? year - date::years{1} : year,
      months.first,
      date::day{1}
    },
    date::year_month_day_last{
      year,
      date::month_day_last(months.second)
    }
  };
}

}  // namespace anime
