/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#ifndef TAIGA_BASE_TIME_H
#define TAIGA_BASE_TIME_H

#include <ctime>
#include <string>
#include <windows.h>

#include "comparable.h"

class Date : public base::Comparable<Date> {
public:
  Date();
  Date(const std::wstring& date);
  Date(unsigned short year, unsigned short month, unsigned short day);
  virtual ~Date() {}

  Date& operator = (const Date& date);

  int operator - (const Date& date) const;

  operator bool() const;
  operator SYSTEMTIME() const;
  operator std::wstring() const;

  unsigned short year;
  unsigned short month;
  unsigned short day;

private:
  base::CompareResult Compare(const Date& date) const;
};

std::wstring GetRelativeTimeString(time_t unix_time);

time_t ConvertIso8601(const std::wstring& datetime);
time_t ConvertRfc822(const std::wstring& datetime);

void GetSystemTime(SYSTEMTIME& st, int utc_offset = 0);

Date GetDate();
std::wstring GetTime(LPCWSTR format = L"HH':'mm':'ss");

Date GetDateJapan();
std::wstring GetTimeJapan(LPCWSTR format = L"HH':'mm':'ss");

std::wstring ToDateString(time_t seconds);
unsigned int ToDayCount(const Date& date);
std::wstring ToTimeString(int seconds);

const Date& EmptyDate();

#endif  // TAIGA_BASE_TIME_H