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

#include "base/chrono.hpp"

namespace anime {

enum class SeasonName { Unknown, Winter, Spring, Summer, Fall };

class Season final {
public:
  Season() = default;
  explicit Season(SeasonName name, std::chrono::year year);
  explicit Season(const Date& date);
  explicit Season(const FuzzyDate& date);

  operator bool() const;
  Season& operator++();
  Season& operator--();
  std::strong_ordering operator<=>(const Season& season) const;

  SeasonName name = SeasonName::Unknown;
  std::chrono::year year{0};
};

}  // namespace anime
