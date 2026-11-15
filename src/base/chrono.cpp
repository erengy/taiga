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

#include "chrono.hpp"

#include <charconv>
#include <format>

namespace base {

Duration::Duration(const seconds_t seconds) : seconds_{seconds} {}

Duration& Duration::operator=(const seconds_t seconds) {
  seconds_ = seconds;
  return *this;
}

Duration::seconds_t::rep Duration::seconds() const {
  return seconds_.count();
}

Duration::minutes_t::rep Duration::minutes() const {
  return std::chrono::duration_cast<minutes_t>(seconds_).count();
}

Duration::hours_t::rep Duration::hours() const {
  return std::chrono::duration_cast<hours_t>(seconds_).count();
}

Duration::days_t::rep Duration::days() const {
  return std::chrono::duration_cast<days_t>(seconds_).count();
}

Duration::weeks_t::rep Duration::weeks() const {
  return std::chrono::duration_cast<weeks_t>(seconds_).count();
}

Duration::months_t::rep Duration::months() const {
  return std::chrono::duration_cast<months_t>(seconds_).count();
}

Duration::years_t::rep Duration::years() const {
  return std::chrono::duration_cast<years_t>(seconds_).count();
}

////////////////////////////////////////////////////////////////////////////////

FuzzyDate::FuzzyDate(std::chrono::year year, std::chrono::month month, std::chrono::day day)
    : year_{year}, month_{month}, day_{day} {}

FuzzyDate::FuzzyDate(const Date& date)
    : year_{date.year()}, month_{date.month()}, day_{date.day()} {}

FuzzyDate::FuzzyDate(const std::string_view date) {
  static constexpr auto toUnsigned = [](const std::string_view str) {
    unsigned_t value{0};
    std::from_chars(str.data(), str.data() + str.size(), value, 10);
    return value;
  };

  // Convert from YYYY-MM-DD
  if (date.size() >= 10) {
    setYear(toUnsigned(date.substr(0, 4)));
    setMonth(toUnsigned(date.substr(5, 2)));
    setDay(toUnsigned(date.substr(8, 2)));
  }
}

FuzzyDate::operator bool() const {
  return year() && month() && day();
}

FuzzyDate::operator Date() const {
  return Date{year_, month_, day_};
}

std::chrono::days FuzzyDate::operator-(const FuzzyDate& date) const {
  return std::chrono::sys_days{static_cast<Date>(*this)} -
         std::chrono::sys_days{static_cast<Date>(date)};
}

std::strong_ordering FuzzyDate::operator<=>(const FuzzyDate& date) const {
  static constexpr auto fuzzyCompare = [](const auto& lhs, const auto& rhs) {
    if (!lhs) return std::strong_ordering::greater;
    if (!rhs) return std::strong_ordering::less;
    return lhs <=> rhs;
  };

  if (year() != date.year()) {
    return fuzzyCompare(year(), date.year());
  }
  if (month() != date.month()) {
    return fuzzyCompare(month(), date.month());
  }
  if (day() != date.day()) {
    return fuzzyCompare(day(), date.day());
  }

  return std::strong_ordering::equal;
}

bool FuzzyDate::operator==(const FuzzyDate& date) const {
  return std::is_eq(*this <=> date);
}

bool FuzzyDate::empty() const {
  return !year() && !month() && !day();
}

std::string FuzzyDate::toString() const {
  return std::format("{:0>4}-{:0>2}-{:0>2}", year(), month(), day());  // YYYY-MM-DD
}

FuzzyDate::unsigned_t FuzzyDate::year() const {
  return static_cast<unsigned_t>(static_cast<int>(year_));
}

FuzzyDate::unsigned_t FuzzyDate::month() const {
  return static_cast<unsigned_t>(static_cast<unsigned int>(month_));
}

FuzzyDate::unsigned_t FuzzyDate::day() const {
  return static_cast<unsigned_t>(static_cast<unsigned int>(day_));
}

void FuzzyDate::setYear(unsigned_t year) {
  year_ = std::chrono::year{year};
}

void FuzzyDate::setMonth(unsigned_t month) {
  month_ = std::chrono::month{month};
}

void FuzzyDate::setDay(unsigned_t day) {
  day_ = std::chrono::day{day};
}

}  // namespace base
