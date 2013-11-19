/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

#ifndef TAIGA_BASE_TYPES_H
#define TAIGA_BASE_TYPES_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "win/http/win_http.h"

// Dictionary types
typedef std::map<std::wstring, std::wstring> dictionary_t;
typedef std::map<std::wstring, std::vector<std::wstring>> multidictionary_t;

// Default enumeration type
typedef unsigned char enum_t;

// Default string type
typedef std::wstring string_t;

// HTTP request and response
typedef win::http::Request HttpRequest;
typedef win::http::Response HttpResponse;

#endif  // TAIGA_BASE_TYPES_H