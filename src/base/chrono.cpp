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

#include "chrono.hpp"

#include <charconv>
#include <format>

namespace base {

FuzzyDate::FuzzyDate(std::chrono::year year, std::chrono::month month, std::chrono::day day)
    : year_{year}, month_{month}, day_{day} {}

FuzzyDate::FuzzyDate(const Date& date)
    : year_{date.year()}, month_{date.month()}, day_{date.day()} {}

FuzzyDate::FuzzyDate(const std::string_view date) {
  static constexpr auto to_unsigned = [](const std::string_view str) {
    unsigned_t value{0};
    std::from_chars(str.data(), str.data() + str.size(), value, 10);
    return value;
  };

  // Convert from YYYY-MM-DD
  if (date.size() >= 10) {
    set_year(to_unsigned(date.substr(0, 4)));
    set_month(to_unsigned(date.substr(5, 2)));
    set_day(to_unsigned(date.substr(8, 2)));
  }
}

FuzzyDate::operator bool() const {
  return year() && month() && day();
}

FuzzyDate::operator Date() const {
  return Date{year_, month_, day_};
}

int FuzzyDate::operator-(const FuzzyDate& date) const {
  const auto days = std::chrono::sys_days{static_cast<Date>(*this)} -
                    std::chrono::sys_days{static_cast<Date>(date)};
  return days.count();
}

std::strong_ordering FuzzyDate::operator<=>(const FuzzyDate& date) const {
  static constexpr auto fuzzy_compare = [](const auto& lhs, const auto& rhs) {
    if (!lhs) return std::strong_ordering::greater;
    if (!rhs) return std::strong_ordering::less;
    return lhs <=> rhs;
  };

  if (year() != date.year()) {
    return fuzzy_compare(year(), date.year());
  }
  if (month() != date.month()) {
    return fuzzy_compare(month(), date.month());
  }
  if (day() != date.day()) {
    return fuzzy_compare(day(), date.day());
  }

  return std::strong_ordering::equal;
}

bool FuzzyDate::empty() const {
  return !year() && !month() && !day();
}

std::string FuzzyDate::to_string() const {
  return std::format("{:0>4}-{:0>2}-{:0>2}", year(), month(), day());  // YYYY-MM-DD
}

FuzzyDate::unsigned_t FuzzyDate::year() const {
  return static_cast<int>(year_);
}

FuzzyDate::unsigned_t FuzzyDate::month() const {
  return static_cast<unsigned>(month_);
}

FuzzyDate::unsigned_t FuzzyDate::day() const {
  return static_cast<unsigned>(day_);
}

void FuzzyDate::set_year(unsigned_t year) {
  year_ = std::chrono::year{year};
}

void FuzzyDate::set_month(unsigned_t month) {
  month_ = std::chrono::month{month};
}

void FuzzyDate::set_day(unsigned_t day) {
  day_ = std::chrono::day{day};
}

}  // namespace base
