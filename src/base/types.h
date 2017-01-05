/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace base {
namespace http {
class Request;
class Response;
}
}

namespace base {

// Unique ID type
typedef std::wstring uid_t;

}  // namespace base

// Default enumeration type
typedef unsigned char enum_t;

// Default string type
typedef std::wstring string_t;

// Dictionary types
typedef std::map<string_t, string_t> dictionary_t;
typedef std::map<string_t, std::vector<string_t>> multidictionary_t;

// HTTP request and response
typedef base::http::Request HttpRequest;
typedef base::http::Response HttpResponse;

// 64-bit integral data type (quadword)
typedef unsigned __int64 QWORD, *LPQWORD;
