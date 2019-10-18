/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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
using uid_t = std::wstring;

}  // namespace base

// Default enumeration type
using enum_t = unsigned char;

// Dictionary types
using dictionary_t = std::map<std::wstring, std::wstring>;
using multidictionary_t = std::map<std::wstring, std::vector<std::wstring>>;

// HTTP request and response
using HttpRequest = base::http::Request;
using HttpResponse = base::http::Response;

// 64-bit integral data type (quadword)
using QWORD = unsigned __int64;
