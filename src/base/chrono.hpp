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

#include <chrono>
#include <compare>
#include <string>
#include <string_view>

namespace base {

using Date = std::chrono::year_month_day;

class Duration {
public:
  explicit Duration(const std::chrono::seconds seconds);

  Duration& operator=(const std::chrono::seconds seconds);

  std::chrono::seconds::rep seconds() const;
  std::chrono::minutes::rep minutes() const;
  std::chrono::hours::rep hours() const;
  std::chrono::days::rep days() const;
  std::chrono::months::rep months() const;
  std::chrono::years::rep years() const;

private:
  std::chrono::seconds seconds_;
};

class FuzzyDate final {
public:
  using unsigned_t = unsigned short;

  FuzzyDate() = default;
  explicit FuzzyDate(std::chrono::year year, std::chrono::month month, std::chrono::day day);
  explicit FuzzyDate(const Date& date);
  explicit FuzzyDate(std::string_view date);

  explicit operator bool() const;
  explicit operator Date() const;
  int operator-(const FuzzyDate& date) const;
  std::strong_ordering operator<=>(const FuzzyDate& date) const;

  [[nodiscard]] bool empty() const;
  [[nodiscard]] std::string to_string() const;

  [[nodiscard]] unsigned_t year() const;
  [[nodiscard]] unsigned_t month() const;
  [[nodiscard]] unsigned_t day() const;

  void set_year(unsigned_t year);
  void set_month(unsigned_t month);
  void set_day(unsigned_t day);

private:
  std::chrono::year year_{};
  std::chrono::month month_{};
  std::chrono::day day_{};
};

}  // namespace base

using base::Date;
using base::Duration;
using base::FuzzyDate;
