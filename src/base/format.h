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

#include <string_view>

#include <fmt/format.h>
#include <fmt/xchar.h>

// `_format` UDL is deprecated. We should eventually move to `fmt::format`
// and then to `std::format` in C++20.
// See: https://github.com/fmtlib/fmt/issues/2640

inline auto operator"" _format(const char* s, size_t n) {
  return [=](auto&&... args) {
    return fmt::format(fmt::runtime(std::string_view(s, n)), args...);
  };
}

inline auto operator"" _format(const wchar_t* s, size_t n) {
  return [=](auto&&... args) {
    return fmt::format(std::wstring_view(s, n), args...);
  };
}
