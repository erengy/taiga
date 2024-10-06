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
#include <ctime>
#include <string>
#include <windows.h>

std::wstring GetAbsoluteTimeString(time_t unix_time, const char* format = nullptr);

time_t ConvertIso8601(const std::wstring& datetime);
time_t ConvertRfc822(const std::wstring& datetime);
std::wstring ConvertRfc822ToLocal(const std::wstring& datetime);

Date GetDate();
Date GetDate(const time_t unix_time);
std::wstring GetTime(LPCWSTR format = L"HH':'mm':'ss");

Date GetDateJapan();

std::wstring ToDateString(Duration duration);
unsigned int ToDayCount(const Date& date);
std::wstring ToTimeString(Duration duration);

const Date& EmptyDate();
