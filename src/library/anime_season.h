/*
** Taiga
** Copyright (C) 2010-2016, Eren Okka
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

#ifndef TAIGA_LIBRARY_ANIME_SEASON_H
#define TAIGA_LIBRARY_ANIME_SEASON_H

#include <string>

#include "base/comparable.h"

class Date;

namespace anime {

class Season : public base::Comparable<Season> {
public:
  enum Name {
    kUnknown,
    kWinter,
    kSpring,
    kSummer,
    kFall
  };

  Season();
  Season(Name name, unsigned short year);
  Season(const Date& date);
  Season(const std::wstring& str);
  ~Season() {}

  Season& operator = (const Season& season);
  Season& operator ++ ();
  Season& operator -- ();

  operator bool() const;

  void GetInterval(Date& date_start, Date& date_end) const;
  std::wstring GetName() const;
  std::wstring GetString() const;

  Name name;
  unsigned short year;

private:
  base::CompareResult Compare(const Season& season) const;
};

}  // namespace anime

#endif  // TAIGA_LIBRARY_ANIME_SEASON_H