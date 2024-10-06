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
#include <QDateTime>
#include <cmath>

#include "base/chrono.hpp"
#include "media/anime.hpp"
#include "media/anime_season.hpp"

namespace gui {

QString formatScore(const double value) {
  return u"%1%2"_qs.arg(value * 10.0).arg('%');
}

QString formatListScore(const int value) {
  return value > 0 ? u"%1"_qs.arg(value / 10.0) : "-";
}

QString formatDate(const base::Date& date) {
  return date.ok() ? QDate(date).toString(Qt::RFC2822Date) : u"?"_qs;
}

QString formatFuzzyDate(const base::FuzzyDate& fuzzyDate) {
  const QDate date(fuzzyDate.year(), fuzzyDate.month(), fuzzyDate.day());
  return fuzzyDate ? date.toString(Qt::RFC2822Date) : u"?"_qs;
}

QString formatAsRelativeTime(const qint64 time) {
  if (!time) return u"Unknown"_qs;

  const QDateTime datetime = QDateTime::fromSecsSinceEpoch(time);
  const QDateTime now = QDateTime::currentDateTimeUtc();
  const auto timeDiff = datetime.secsTo(now);
  const Duration duration(std::chrono::seconds{std::abs(timeDiff)});

  const QString str = [&duration]() {
    if (duration.seconds() < 90) return u"a moment"_qs;
    if (duration.minutes() < 45) return u"%1 minute(s)"_qs.arg(std::lround(duration.minutes()));
    if (duration.hours() < 22) return u"%1 hour(s)"_qs.arg(std::lround(duration.hours()));
    if (duration.days() < 25) return u"%1 day(s)"_qs.arg(std::lround(duration.days()));
    if (duration.days() < 345) return u"%1 month(s)"_qs.arg(std::lround(duration.months()));
    return u"%1 year(s)"_qs.arg(std::lround(duration.years()));
  }();

  return timeDiff < 0 ? u"in %1"_qs.arg(str) : u"%1 ago"_qs.arg(str);
}

QString formatSeason(const anime::Season season) {
  return u"%1 %2"_qs.arg(formatSeasonName(season.name)).arg(static_cast<int>(season.year));
}

QString formatSeasonName(const anime::SeasonName name) {
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

QString formatStatus(const anime::Status value) {
  using enum anime::Status;

  // clang-format off
  switch (value) {
    case Unknown: return "Unknown";
    case Airing: return "Airing";
    case FinishedAiring: return "Finished";
    case NotYetAired: return "Not yet aired";
    default: return "";
  }
  // clang-format on
}

QString formatType(const anime::Type value) {
  using enum anime::Type;

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

QString formatListStatus(const anime::list::Status value) {
  using enum anime::list::Status;

  // clang-format off
  switch (value) {
    case NotInList: return "Not in list";
    case Watching: return "Watching";
    case Completed: return "Completed";
    case OnHold: return "Paused";
    case Dropped: return "Dropped";
    case PlanToWatch: return "Planning";
    default: return "";
  }
  // clang-format on
}

}  // namespace gui
