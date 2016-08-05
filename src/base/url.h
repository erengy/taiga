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

#ifndef TAIGA_BASE_URL_H
#define TAIGA_BASE_URL_H

#include <string>

#include "map.h"

typedef base::multimap<std::wstring, std::wstring> query_t;

namespace base {
namespace http {
enum Protocol {
  kHttp,
  kHttps,
  kRelative,
};
}
}

class Url {
public:
  Url();
  Url(const std::wstring& url);
  ~Url() {}

  void Clear();
  std::wstring Build() const;
  void Crack(std::wstring url);

  Url& operator=(const Url& url);
  void operator=(const std::wstring& url);

  base::http::Protocol protocol;
  std::wstring host;
  unsigned short port;
  std::wstring path;
  query_t query;
  std::wstring fragment;
};

std::wstring BuildUrlParameters(const query_t& parameters);
std::wstring DecodeUrl(const std::wstring& input);
std::wstring EncodeUrl(const std::wstring& input, bool encode_unreserved = false);

#endif  // TAIGA_BASE_URL_H