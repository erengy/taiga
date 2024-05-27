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
#include <ctime>
#include <string>
#include <windows.h>

class Duration {
public:
  using seconds_t = std::chrono::seconds;
  using minutes_t = std::chrono::duration
      <float, std::ratio<60>>;
  using hours_t = std::chrono::duration
      <float, std::ratio_multiply<std::ratio<60>, minutes_t::period>>;
  using days_t = std::chrono::duration
      <float, std::ratio_multiply<std::ratio<24>, hours_t::period>>;
  using years_t = std::chrono::duration
      <float, std::ratio_multiply<std::ratio<146097, 400>, days_t::period>>;
  using months_t = std::chrono::duration
      <float, std::ratio_divide<years_t::period, std::ratio<12>>>;

  Duration(const seconds_t seconds);
  Duration(const std::time_t seconds);

  Duration& operator=(const seconds_t seconds);
  Duration& operator=(const std::time_t seconds);

  seconds_t::rep seconds() const;
  minutes_t::rep minutes() const;
  hours_t::rep hours() const;
  days_t::rep days() const;
  months_t::rep months() const;
  years_t::rep years() const;

private:
  seconds_t seconds_;
};

std::wstring GetAbsoluteTimeString(time_t unix_time, const char* format = nullptr);
std::wstring GetRelativeTimeString(time_t unix_time, bool append_suffix);

time_t ConvertIso8601(const std::wstring& datetime);
time_t ConvertRfc822(const std::wstring& datetime);
std::wstring ConvertRfc822ToLocal(const std::wstring& datetime);

Date GetDate();
Date GetDate(const time_t unix_time);
std::wstring GetTime(LPCWSTR format = L"HH':'mm':'ss");

Date GetDateJapan();

std::wstring ToDateString(Duration duration);
unsigned int ToDayCount(const Date& date);
std::wstring ToTimeString(Duration duration);

const Date& EmptyDate();
