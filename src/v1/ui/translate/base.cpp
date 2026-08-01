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

#include <algorithm>
#include <array>
#include <string>

#include "ui/translate.h"

#include "base/format.h"
#include "base/time.h"
#include "base/string.h"
#include "media/anime_util.h"

namespace ui {

std::wstring TranslateDate(const Date& date) {
  if (!anime::IsValidDate(date))
    return L"?";

  std::wstring result;

  if (date.month() > 0 && date.month() <= 12) {
    result += TranslateMonth(date.month()) + L" ";
  }
  if (date.day() > 0) {
    result += ToWstr(date.day()) + L", ";
  }
  result += ToWstr(date.year());

  return result;
}

std::wstring TranslateDateRange(const std::pair<Date, Date>& range) {
  const auto& [a, b] = range;
  return L"{} {} to {} {}"_format(TranslateMonth(a.month()), a.year(),
                                  TranslateMonth(b.month()), b.year());
}

std::wstring TranslateMonth(const int month) {
  constexpr std::array<wchar_t*, 12> months{
        L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
        L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
    };

  return months[std::clamp(month - 1, 0, 11)];
}

std::wstring TranslateNumber(const int value,
                             const std::wstring& default_char) {
  return value > 0 ? ToWstr(value) : default_char;
}

}  // namespace ui
