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

#include <cmath>
#include <regex>

#include "base/time.h"

#include "base/format.h"
#include "base/string.h"

static long GetTimeZoneBias() {
  TIME_ZONE_INFORMATION tz_info = {0};
  const auto tz_id = ::GetTimeZoneInformation(&tz_info);
  return tz_info.Bias + tz_info.DaylightBias;
}

static void NeutralizeTimezone(tm& t) {
  static bool initialized = false;
  static long timezone_difference = 0;

  if (!initialized) {
    _tzset();
    _get_timezone(&timezone_difference);

    long dst_difference = 0;
    _get_dstbias(&dst_difference);
    timezone_difference += dst_difference;

    initialized = true;
  }

  t.tm_sec -= timezone_difference;  // mktime uses the current time zone
}

time_t ConvertIso8601(const std::wstring& datetime) {
  // e.g.
  // "2015-02-20T04:43:50"
  // "2015-02-20T04:43:50.016Z"
  // "2015-02-20T06:43:50.016+02:00"
  static const std::wregex pattern(
      L"(\\d{4})-(\\d{2})-(\\d{2})"
      L"T(\\d{2}):(\\d{2}):(\\d{2})(?:[.,]\\d+)?"
      L"(?:(?:([+-])(\\d{2}):(\\d{2}))|Z)?");

  std::match_results<std::wstring::const_iterator> m;
  time_t result = -1;

  if (std::regex_match(datetime, m, pattern)) {
    tm t = {0};
    t.tm_year = ToInt(m[1].str()) - 1900;
    t.tm_mon = ToInt(m[2].str()) - 1;
    t.tm_mday = ToInt(m[3].str());
    t.tm_hour = ToInt(m[4].str());
    t.tm_min = ToInt(m[5].str());
    t.tm_sec = ToInt(m[6].str());

    if (m[7].matched) {
      int sign = m[7].str() == L"+" ? 1 : -1;
      t.tm_hour += sign * ToInt(m[8].str());
      t.tm_min += sign * ToInt(m[9].str());
    }
    NeutralizeTimezone(t);
    t.tm_isdst = -1;

    result = std::mktime(&t);
  }

  return result;
}

time_t ConvertRfc822(const std::wstring& datetime) {
  // See: https://tools.ietf.org/html/rfc822#section-5
  static const std::wregex pattern(
      L"(?:(Mon|Tue|Wed|Thu|Fri|Sat|Sun), )?"
      L"(\\d{1,2}) (Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) (\\d{2,4}) "
      L"(\\d{2}):(\\d{2})(?::(\\d{2}))? "
      L"(UT|GMT|EST|EDT|CST|CDT|MST|MDT|PST|PDT|[ZAMNY]|[+-]\\d{4})");

  static const std::vector<std::wstring> months{
    L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
    L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
  };

  std::match_results<std::wstring::const_iterator> m;
  time_t result = -1;

  if (std::regex_match(datetime, m, pattern)) {
    tm t = {0};

    t.tm_mday = ToInt(m[2].str());
    t.tm_mon = static_cast<int>(std::distance(months.begin(),
        std::find(months.begin(), months.end(), m[3].str())));
    t.tm_year = ToInt(m[4].str());
    if (t.tm_year > 1900)
      t.tm_year -= 1900;

    t.tm_hour = ToInt(m[5].str());
    t.tm_min = ToInt(m[6].str());
    if (m[7].matched)
      t.tm_sec = ToInt(m[7].str());

    // TODO: Handle other time zones
    if (StartsWith(m[8].str(), L"+") || StartsWith(m[8].str(), L"-")) {
      int sign = StartsWith(m[8].str(), L"+") ? 1 : -1;
      t.tm_hour += sign * ToInt(m[8].str().substr(1, 3));
      t.tm_min += sign * ToInt(m[8].str().substr(3, 5));
    }
    NeutralizeTimezone(t);
    t.tm_isdst = -1;

    result = std::mktime(&t);
  }

  return result;
}

std::wstring ConvertRfc822ToLocal(const std::wstring& datetime) {
  auto time = ConvertRfc822(datetime);

  std::tm local_tm = {0};
  if (localtime_s(&local_tm, &time) != 0)
    return datetime;

  std::string result(100, '\0');
  std::strftime(&result.at(0), result.size(),
                "%a, %d %b %Y %H:%M:%S", &local_tm);

  const auto bias = GetTimeZoneBias();

  const auto sign = bias <= 0 ? L'+' : L'-';
  const auto hh = std::abs(bias) / 60;
  const auto mm = std::abs(bias) % 60;

  return L"{} {}{:0>2}{:0>2}"_format(StrToWstr(result), sign, hh, mm);
}

std::wstring GetAbsoluteTimeString(time_t unix_time, const char* format) {
  std::tm tm;

  if (!unix_time || localtime_s(&tm, &unix_time))
    return L"Unknown";

  Duration duration(std::abs(time(nullptr) - unix_time));
  Date today = GetDate();

  auto strftime = [&tm](const char* format) {
    std::string result(100, '\0');
    std::strftime(&result.at(0), result.size(), format, &tm);
    return StrToWstr(result);
  };

  if (format) {
    return strftime(format);
  }

  if (1900 + tm.tm_year < today.year()) {
    return strftime("%d %B %Y");  // 01 January 2014
  } else if (std::lround(duration.days()) <= 1 && tm.tm_mday == today.day()) {
    return strftime("%H:%M");  // 13:37
  } else if (std::lround(duration.days()) <= 7) {
    return strftime("%A, %d %B");  // Thursday, 01 January
  } else {
    return strftime("%d %B");  // 01 January
  }
}

Date GetDate() {
  SYSTEMTIME st;
  ::GetLocalTime(&st);
  return Date(st);
}

Date GetDate(const time_t unix_time) {
  std::tm tm;

  if (localtime_s(&tm, &unix_time) == 0) {
    return Date(1900 + tm.tm_year, tm.tm_mon + 1, tm.tm_mday);
  }

  return Date();
}

std::wstring GetTime(LPCWSTR format) {
  WCHAR buff[32];
  GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, NULL, format, buff, 32);
  return buff;
}

Date GetDateJapan() {
  const auto utc = std::chrono::system_clock::now();
  // Japan Standard Time is UTC+09. There is no daylight saving time in Japan.
  const auto jst = utc + std::chrono::hours(9);
  return Date(std::chrono::floor<date::days>(jst));
}

std::wstring ToDateString(Duration duration) {
  std::wstring date;

  if (duration.seconds() > 0) {
    const auto days = static_cast<int>(duration.days());
    duration = duration.seconds() % Duration::days_t::period::num;

    const auto hours = static_cast<int>(duration.hours());
    duration = duration.seconds() % Duration::hours_t::period::num;

    const auto minutes = static_cast<int>(duration.minutes());
    duration = duration.seconds() % Duration::minutes_t::period::num;

    auto add_time = [&date](const int value, const std::wstring& str) {
      if (value > 0) {
        date += (!date.empty() ? L" " : L"") +
                ToWstr(value) + L" " + str + (value > 1 ? L"s" : L"");
      }
    };

    add_time(days, L"day");
    add_time(hours, L"hour");
    add_time(minutes, L"minute");
    add_time(static_cast<int>(duration.seconds()), L"second");
  }

  return date;
}

unsigned int ToDayCount(const Date& date) {
  const auto days = date::sys_days{static_cast<date::year_month_day>(date)};
  return days.time_since_epoch().count();
}

std::wstring ToTimeString(Duration duration) {
  const auto hours = static_cast<int>(duration.hours());
  duration = duration.seconds() % Duration::hours_t::period::num;

  const auto minutes = static_cast<int>(duration.minutes());
  duration = duration.seconds() % Duration::minutes_t::period::num;

  return (hours > 0 ? PadChar(ToWstr(hours), '0', 2) + L":" : L"") +
         PadChar(ToWstr(minutes), '0', 2) + L":" +
         PadChar(ToWstr(duration.seconds()), '0', 2);
}

const Date& EmptyDate() {
  static const Date date;
  return date;
}
