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

#ifndef TAIGA_BASE_BASE64_H
#define TAIGA_BASE_BASE64_H

#include <string>

std::string Base64Decode(const std::string& str);
std::string Base64Encode(const std::string& str);

std::wstring Base64Decode(const std::wstring& str, bool for_filename = false);
std::wstring Base64Encode(const std::wstring& str, bool for_filename = false);

#endif  // TAIGA_BASE_BASE64_H