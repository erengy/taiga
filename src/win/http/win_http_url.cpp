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

#include "win_http.h"

#include "base/string.h"

namespace win {
namespace http {

Url::Url(const std::wstring& url) {
  Crack(url);
}

Url& Url::operator=(const Url& url) {
  scheme = url.scheme;
  host = url.host;
  path = url.path;

  return *this;
}

void Url::operator=(const std::wstring& url) {
  Crack(url);
}

void Url::Crack(std::wstring url) {
  // Get scheme
  size_t i = url.find(L"://", 0);
  if (i != std::wstring::npos) {
    scheme = url.substr(0, i);
    url = url.substr(i + 3);
  } else {
    scheme.clear();
  }

  // Get host and path
  i = url.find(L"/", 0);
  if (i == std::wstring::npos)
    i = url.length();
  host = url.substr(0, i);
  path = url.substr(i);
}

}  // namespace http
}  // namespace win