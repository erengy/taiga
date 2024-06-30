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

#pragma once

#include <QString>
#include <chrono>

namespace base {

using Date = std::chrono::year_month_day;
class FuzzyDate;

}  // namespace base

namespace anime {

enum class SeasonName;
enum class Status;
enum class Type;

class Season;

namespace list {

enum class Status;

}

}  // namespace anime

namespace gui {

QString formatScore(const double value);

QString fromDate(const base::Date& date);
QString fromFuzzyDate(const base::FuzzyDate& date);

QString fromSeason(const anime::Season season);
QString fromSeasonName(const anime::SeasonName name);
QString fromStatus(const anime::Status value);
QString fromType(const anime::Type value);

QString fromListStatus(const anime::list::Status value);

}  // namespace gui
