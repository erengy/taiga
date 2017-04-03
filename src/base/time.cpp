/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cmath>
#include <regex>

#include "string.h"
#include "time.h"

Date::Date()
    : Date(0, 0, 0) {
}

Date::Date(const std::wstring& date)
    : Date(0, 0, 0) {
  // Convert from YYYY-MM-DD
  if (date.length() >= 10) {
    set_year(ToInt(date.substr(0, 4)));
    set_month(ToInt(date.substr(5, 2)));
    set_day(ToInt(date.substr(8, 2)));
  }
}

Date::Date(const SYSTEMTIME& st)
    : year_(st.wYear), month_(st.wMonth), day_(st.wDay) {
}

Date::Date(unsigned short year, unsigned short month, unsigned short day)
    : year_(year), month_(month), day_(day) {
}

Date& Date::operator=(const Date& date) {
  year_ = date.year_;
  month_ = date.month_;
  day_ = date.day_;

  return *this;
}

int Date::operator-(const Date& date) const {
  const auto days = date::sys_days{static_cast<date::year_month_day>(*this)} -
                    date::sys_days{static_cast<date::year_month_day>(date)};
  return days.count();
}

Date::operator bool() const {
  return year() && month() && day();
}

Date::operator SYSTEMTIME() const {
  SYSTEMTIME st = {0};
  st.wYear = static_cast<WORD>(year());
  st.wMonth = static_cast<WORD>(month());
  st.wDay = static_cast<WORD>(day());

  return st;
}

Date::operator std::wstring() const {
  return to_string();
}

Date::operator date::year_month_day() const {
  return date::year_month_day{year_, month_, day_};
}

std::wstring Date::to_string() const {
  // Convert to YYYY-MM-DD
  return PadChar(ToWstr(year()), '0', 4) + L"-" +
         PadChar(ToWstr(month()), '0', 2) + L"-" +
         PadChar(ToWstr(day()), '0', 2);
}

unsigned short Date::year() const {
  return static_cast<int>(year_);
}

unsigned short Date::month() const {
  return static_cast<unsigned>(month_);
}

unsigned short Date::day() const {
  return static_cast<unsigned>(day_);
}

void Date::set_year(unsigned short year) {
  year_ = date::year{year};
}

void Date::set_month(unsigned short month) {
  month_ = date::month{month};
}

void Date::set_day(unsigned short day) {
  day_ = date::day{day};
}

base::CompareResult Date::Compare(const Date& date) const {
  if (year_ != date.year_) {
    if (year() == 0)
      return base::kGreaterThan;
    if (date.year() == 0)
      return base::kLessThan;
    return year_ < date.year_ ? base::kLessThan : base::kGreaterThan;
  }

  if (month_ != date.month_) {
    if (month() == 0)
      return base::kGreaterThan;
    if (date.month() == 0)
      return base::kLessThan;
    return month_ < date.month_ ? base::kLessThan : base::kGreaterThan;
  }

  if (day_ != date.day_) {
    if (day() == 0)
      return base::kGreaterThan;
    if (date.day() == 0)
      return base::kLessThan;
    return day_ < date.day_ ? base::kLessThan : base::kGreaterThan;
  }

  return base::kEqualTo;
}

////////////////////////////////////////////////////////////////////////////////

Duration::Duration(const seconds_t seconds)
    : seconds_(seconds) {
}

Duration::Duration(const std::time_t seconds)
    : Duration(std::chrono::seconds(seconds)) {
}

Duration& Duration::operator=(const seconds_t seconds) {
  seconds_ = seconds;
  return *this;
}

Duration& Duration::operator=(const std::time_t seconds) {
  seconds_ = static_cast<seconds_t>(seconds);
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

Duration::months_t::rep Duration::months() const {
  return std::chrono::duration_cast<months_t>(seconds_).count();
}

Duration::years_t::rep Duration::years() const {
  return std::chrono::duration_cast<years_t>(seconds_).count();
}

////////////////////////////////////////////////////////////////////////////////

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
    t.tm_mon = std::distance(months.begin(),
        std::find(months.begin(), months.end(), m[3].str()));
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

  TIME_ZONE_INFORMATION time_zone_info = {0};
  const auto time_zone_id = GetTimeZoneInformation(&time_zone_info);
  const auto bias = time_zone_info.Bias + time_zone_info.DaylightBias;

  std::wstring sign = bias <= 0 ? L"+" : L"-";
  int hh = std::abs(bias) / 60;
  int mm = std::abs(bias) % 60;

  std::wstring result_with_tz = StrToWstr(result) + L" " + sign +
      PadChar(ToWstr(hh), L'0', 2) +
      PadChar(ToWstr(mm), L'0', 2);

  return result_with_tz;
}

std::wstring GetAbsoluteTimeString(time_t unix_time) {
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

std::wstring GetRelativeTimeString(time_t unix_time, bool append_suffix) {
  if (!unix_time)
    return L"Unknown";

  time_t time_diff = time(nullptr) - unix_time;
  Duration duration(std::abs(time_diff));
  bool future = time_diff < 0;

  std::wstring str;

  auto str_value = [](const float value,
                      const std::wstring& singular,
                      const std::wstring& plural) {
    long result = std::lround(value);
    return ToWstr(result) + L" " + (result == 1 ? singular : plural);
  };

  if (duration.seconds() < 90) {
    str = L"a moment";
  } else if (duration.minutes() < 45) {
    str = str_value(duration.minutes(), L"minute", L"minutes");
  } else if (duration.hours() < 22) {
    str = str_value(duration.hours(), L"hour", L"hours");
  } else if (duration.days() < 25) {
    str = str_value(duration.days(), L"day", L"days");
  } else if (duration.days() < 345) {
    str = str_value(duration.months(), L"month", L"months");
  } else {
    str = str_value(duration.years(), L"year", L"years");
  }

  if (append_suffix) {
    if (future) {
      str = L"in " + str;
    } else {
      str += L" ago";
    }
  }

  return str;
}

void GetSystemTime(SYSTEMTIME& st, int utc_offset) {
  // Get current time, expressed in UTC
  GetSystemTime(&st);
  if (utc_offset == 0)
    return;

  // Convert to FILETIME
  FILETIME ft;
  SystemTimeToFileTime(&st, &ft);
  // Convert to ULARGE_INTEGER
  ULARGE_INTEGER ul;
  ul.LowPart = ft.dwLowDateTime;
  ul.HighPart = ft.dwHighDateTime;

  // Apply UTC offset
  ul.QuadPart += static_cast<ULONGLONG>(utc_offset) * 60 * 60 * 10000000;

  // Convert back to SYSTEMTIME
  ft.dwLowDateTime = ul.LowPart;
  ft.dwHighDateTime = ul.HighPart;
  FileTimeToSystemTime(&ft, &st);
}

Date GetDate() {
  SYSTEMTIME st;
  GetLocalTime(&st);
  return Date(st);
}

Date GetDate(time_t unix_time) {
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
  SYSTEMTIME st;
  GetSystemTime(st, 9);  // JST is UTC+09
  return Date(st);
}

std::wstring GetTimeJapan(LPCWSTR format) {
  WCHAR buff[32];
  SYSTEMTIME st_jst;
  GetSystemTime(st_jst, 9);  // JST is UTC+09
  GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &st_jst, format, buff, 32);
  return buff;
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
