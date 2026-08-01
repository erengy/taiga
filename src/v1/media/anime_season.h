/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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

#pragma once

#include <array>
#include <string>

#include <nstd/compare.hpp>

#include "base/time.h"

namespace anime {

class Season final : public nstd::Comparable<Season> {
public:
  enum class Name {
    Unknown,
    Winter,
    Spring,
    Summer,
    Fall
  };

  Season() = default;
  explicit Season(Name name, date::year year) : name{name}, year{year} {}
  explicit Season(const DateFull& date) : Season{Date{date}} {}
  explicit Season(const Date& date);
  explicit Season(const std::string& str);

  operator bool() const;

  Season& operator++();
  Season& operator--();

  int compare(const Season& season) const override;

  std::pair<DateFull, DateFull> to_date_range() const;

  Name name = Name::Unknown;
  date::year year{0};
};

}  // namespace anime
