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

#include "string.h"
#include "url.h"

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

////////////////////////////////////////////////////////////////////////////////

std::wstring EncodeUrl(const std::wstring& input, bool encode_unreserved) {
  std::string str = WstrToStr(input);

  std::wstring output;
  output.reserve(input.size() * 2);

  for (size_t i = 0; i < str.length(); i++) {
    if ((str[i] >= '0' && str[i] <= '9') ||
        (str[i] >= 'A' && str[i] <= 'Z') ||
        (str[i] >= 'a' && str[i] <= 'z') ||
        (!encode_unreserved &&
         (str[i] == '-' || str[i] == '.' ||
          str[i] == '_' || str[i] == '~'))) {
      output.append(1, static_cast<wchar_t>(str[i]));
    } else {
      static const wchar_t* digits = L"0123456789ABCDEF";
      output.append(L"%");
      output.append(&digits[(str[i] >> 4) & 0x0F], 1);
      output.append(&digits[str[i] & 0x0F], 1);
    }
  }

  return output;
}