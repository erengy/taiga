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

#ifndef TIME_H
#define TIME_H

#include "std.h"
#include <ctime>

// =============================================================================

enum TimerId {
  TIMER_MAIN = 1337,
  TIMER_TAIGA = 74164
};

class Date {
 public:
  Date();
  Date(const wstring& date);
  Date(unsigned short year, unsigned short month, unsigned short day);
  virtual ~Date() {}

  Date& operator = (const Date& date);

  bool operator == (const Date& date) const;
  bool operator != (const Date& date) const;
  bool operator <  (const Date& date) const;
  bool operator <= (const Date& date) const;
  bool operator >= (const Date& date) const;
  bool operator >  (const Date& date) const;

  int operator - (const Date& date) const;

  operator bool() const;
  operator SYSTEMTIME() const;
  operator wstring() const;

  unsigned short year;
  unsigned short month;
  unsigned short day;
};

void GetSystemTime(SYSTEMTIME& st, int utc_offset = 0);

Date GetDate();
wstring GetTime(LPCWSTR lpFormat = L"HH':'mm':'ss");

Date GetDateJapan();
wstring GetTimeJapan(LPCWSTR lpFormat = L"HH':'mm':'ss");

wstring ToDateString(time_t seconds);
unsigned int ToDayCount(const Date& date);
wstring ToTimeString(int seconds);

#endif // TIME_H