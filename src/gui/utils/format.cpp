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

#include "format.hpp"

#include <QDate>

#include "base/chrono.hpp"
#include "media/anime.hpp"
#include "media/anime_season.hpp"

namespace gui {

QString formatScore(const double value) {
  return u"%1%2"_qs.arg(value * 10.0).arg('%');
}

QString fromDate(const base::Date& date) {
  return QDate(static_cast<int>(date.year()), static_cast<unsigned int>(date.month()),
               static_cast<unsigned int>(date.day()))
      .toString();
}

QString fromFuzzyDate(const base::FuzzyDate& date) {
  return QDate(date.year(), date.month(), date.day()).toString();
}

QString fromSeason(const anime::Season season) {
  return u"%1 %2"_qs.arg(fromSeasonName(season.name)).arg(static_cast<int>(season.year));
}

QString fromSeasonName(const anime::SeasonName name) {
  using enum anime::SeasonName;

  // clang-format off
  switch (name) {
    default:
    case Unknown: return "Unknown";
    case Winter: return "Winter";
    case Spring: return "Spring";
    case Summer: return "Summer";
    case Fall: return "Fall";
  }
  // clang-format on
}

QString fromStatus(const anime::SeriesStatus value) {
  using enum anime::SeriesStatus;

  // clang-format off
  switch (value) {
    case Unknown: return "Unknown";
    case Airing: return "Currently airing";
    case FinishedAiring: return "Finished airing";
    case NotYetAired: return "Not yet aired";
    default: return "";
  }
  // clang-format on
}

QString fromType(const anime::SeriesType value) {
  using enum anime::SeriesType;

  // clang-format off
  switch (value) {
    case Unknown: return "Unknown";
    case Tv: return "TV";
    case Ova: return "OVA";
    case Movie: return "Movie";
    case Special: return "Special";
    case Ona: return "ONA";
    case Music: return "Music";
    default: return "";
  }
  // clang-format on
}

}  // namespace gui
