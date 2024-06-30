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

#include <string>

#include "base/time.h"
#include "media/anime.h"

namespace ui {

std::wstring TranslateDate(const Date& date);
std::wstring TranslateDateRange(const std::pair<Date, Date>& range);
std::wstring TranslateMonth(const int month);
std::wstring TranslateNumber(const int value, const std::wstring& default_char = L"-");

std::wstring TranslateScore(const double value);
anime::SeriesType TranslateType(const std::wstring& value);

std::wstring TranslateDateToSeasonString(const Date& date);
std::wstring TranslateSeasonToMonths(const anime::Season& season);

std::wstring TranslateMyDate(const Date& value, const std::wstring& default_char = L"-");
std::wstring TranslateMyScore(const int value, const std::wstring& default_char = L"-");
std::wstring TranslateMyScoreFull(const int value);

}  // namespace ui
