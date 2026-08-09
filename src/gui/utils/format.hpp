/**
 * Taiga
 * Copyright (C) 2010-2026, Eren Okka
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

#include <QDate>
#include <QString>
#include <chrono>

namespace base {

using Date = std::chrono::year_month_day;
class Duration;
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

QString formatNumber(const int value, QString placeholder = "-");

QString formatEpisodeLength(const int minutes, QString placeholder = "-");

QString formatScore(const double value);
QString formatListScore(const int value, QString placeholder = "-");

QString formatDate(const base::Date& date, QString placeholder = "?");
QString formatDate(const QDate date, QString placeholder = "?");
QString formatFuzzyDate(const base::FuzzyDate& date, QString placeholder = "?");
QString formatFuzzyDateRange(const base::FuzzyDate& from, const base::FuzzyDate& to,
                             QString placeholder = "?");
QString formatAsRelativeTime(const qint64 time, QString placeholder = "Unknown");
QString formatDuration(base::Duration duration);
QString formatTimestamp(const qint64 time);
QString formatTransferProgress(const qint64 current, const qint64 total);

QString formatSeason(const anime::Season season, QString placeholder = "Unknown");
QString formatSeasonName(const anime::SeasonName name);
QString formatStatus(const anime::Status value);
QString formatType(const anime::Type value);

QString formatListStatus(const anime::list::Status value);

}  // namespace gui
