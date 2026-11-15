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

#include <chrono>
#include <compare>
#include <string>
#include <string_view>

namespace base {

using Date = std::chrono::year_month_day;

class Duration {
public:
  using seconds_t = std::chrono::seconds;
  using minutes_t = std::chrono::duration<float, std::chrono::minutes::period>;
  using hours_t = std::chrono::duration<float, std::chrono::hours::period>;
  using days_t = std::chrono::duration<float, std::chrono::days::period>;
  using weeks_t = std::chrono::duration<float, std::chrono::weeks::period>;
  using months_t = std::chrono::duration<float, std::chrono::months::period>;
  using years_t = std::chrono::duration<float, std::chrono::years::period>;

  explicit Duration(const seconds_t seconds);

  Duration& operator=(const seconds_t seconds);

  seconds_t::rep seconds() const;
  minutes_t::rep minutes() const;
  hours_t::rep hours() const;
  days_t::rep days() const;
  weeks_t::rep weeks() const;
  months_t::rep months() const;
  years_t::rep years() const;

private:
  seconds_t seconds_;
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
  std::chrono::days operator-(const FuzzyDate& date) const;
  std::strong_ordering operator<=>(const FuzzyDate& date) const;
  bool operator==(const FuzzyDate& date) const;

  [[nodiscard]] bool empty() const;
  [[nodiscard]] std::string toString() const;

  [[nodiscard]] unsigned_t year() const;
  [[nodiscard]] unsigned_t month() const;
  [[nodiscard]] unsigned_t day() const;

  void setYear(unsigned_t year);
  void setMonth(unsigned_t month);
  void setDay(unsigned_t day);

private:
  std::chrono::year year_{};
  std::chrono::month month_{};
  std::chrono::day day_{};
};

}  // namespace base

using base::Date;
using base::Duration;
using base::FuzzyDate;
