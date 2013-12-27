/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#include "string.h"
#include "time.h"

Date::Date()
    : year(0), month(0), day(0) {
}

Date::Date(const std::wstring& date)
    : year(0), month(0), day(0) {
  // Convert from YYYY-MM-DD
  if (date.length() >= 10) {
    year = ToInt(date.substr(0, 4));
    month = ToInt(date.substr(5, 2));
    day = ToInt(date.substr(8, 2));
  }
}

Date::Date(unsigned short year, unsigned short month, unsigned short day)
    : year(year), month(month), day(day) {
}

Date& Date::operator = (const Date& date) {
  year = date.year;
  month = date.month;
  day = date.day;

  return *this;
}

bool Date::operator == (const Date& date) const {
  return year == date.year && month == date.month && day == date.day;
}

bool Date::operator != (const Date& date) const {
  return !operator == (date);
}

bool Date::operator < (const Date& date) const {
  if (year && !date.year) return true;
  if (!year && date.year) return false;
  if (year != date.year) return year < date.year;

  if (month && !date.month) return true;
  if (!month && date.month) return false;
  if (month != date.month) return month < date.month;

  if (day && !date.day) return true;
  if (!day && date.day) return false;
  return day < date.day;
}

bool Date::operator <= (const Date& date) const {
  return !operator > (date);
}

bool Date::operator >= (const Date& date) const {
  return !operator < (date);
}

bool Date::operator > (const Date& date) const {
  if (!year && date.year) return true;
  if (year && !date.year) return false;
  if (year != date.year) return year > date.year;

  if (!month && date.month) return true;
  if (month && !date.month) return false;
  if (month != date.month) return month > date.month;

  if (!day && date.day) return true;
  if (day && !date.day) return false;
  return day > date.day;
}

int Date::operator - (const Date& date) const {
  return ((year * 365) + (month * 30) + day) -
         ((date.year * 365) + (date.month * 30) + date.day);
}

Date::operator bool() const {
  return year != 0 || month != 0 || day != 0;
}

Date::operator SYSTEMTIME() const {
  SYSTEMTIME st;
  st.wYear = year;
  st.wMonth = month;
  st.wDay = day;

  return st;
}

Date::operator std::wstring() const {
  // Convert to YYYY-MM-DD
  return PadChar(ToWstr(year), '0', 4) + L"-" +
         PadChar(ToWstr(month), '0', 2) + L"-" +
         PadChar(ToWstr(day), '0', 2);
}

////////////////////////////////////////////////////////////////////////////////

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
  return Date(st.wYear, st.wMonth, st.wDay);
}

std::wstring GetTime(LPCWSTR format) {
  WCHAR buff[32];
  GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, NULL, format, buff, 32);
  return buff;
}

Date GetDateJapan() {
  SYSTEMTIME st_jst;
  GetSystemTime(st_jst, 9);  // JST is UTC+09
  return Date(st_jst.wYear, st_jst.wMonth, st_jst.wDay);
}

std::wstring GetTimeJapan(LPCWSTR format) {
  WCHAR buff[32];
  SYSTEMTIME st_jst;
  GetSystemTime(st_jst, 9);  // JST is UTC+09
  GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &st_jst, format, buff, 32);
  return buff;
}

std::wstring ToDateString(time_t seconds) {
  time_t days, hours, minutes;
  std::wstring date;

  if (seconds > 0) {
    #define CALC_TIME(x, y) x = seconds / (y); seconds = seconds % (y);
    CALC_TIME(days, 60 * 60 * 24);
    CALC_TIME(hours, 60 * 60);
    CALC_TIME(minutes, 60);
    #undef CALC_TIME
    date.clear();
    #define ADD_TIME(x, y) \
      if (x > 0) { \
        if (!date.empty()) date += L" "; \
        date += ToWstr(x) + y; \
        if (x > 1) date += L"s"; \
      }
    ADD_TIME(days, L" day");
    ADD_TIME(hours, L" hour");
    ADD_TIME(minutes, L" minute");
    ADD_TIME(seconds, L" second");
    #undef ADD_TIME
  }

  return date;
}

unsigned int ToDayCount(const Date& date) {
  return (date.year * 365) + (date.month * 30) + date.day;
}

std::wstring ToTimeString(int seconds) {
  int hours = seconds / 3600;
  seconds = seconds % 3600;
  int minutes = seconds / 60;
  seconds = seconds % 60;

  #define TWO_DIGIT(x) (x >= 10 ? ToWstr(x) : L"0" + ToWstr(x))
  return (hours > 0 ? TWO_DIGIT(hours) + L":" : L"") +
         TWO_DIGIT(minutes) + L":" + TWO_DIGIT(seconds);
  #undef TWO_DIGIT
}

const Date& EmptyDate() {
  static const Date date;
  return date;
}