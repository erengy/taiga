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

#include <string>

#include <base64/base64.h>

#include "base64.h"
#include "string.h"

std::wstring Base64Decode(const std::string& str, bool for_filename) {
  if (str.empty())
    return EmptyString();

  Base64Coder coder;
  coder.Decode((BYTE*)str.c_str(), str.size());

  if (for_filename) {
    std::wstring msg = StrToWstr(coder.DecodedMessage());
    ReplaceChar(msg, '-', '/');
    return msg;
  } else {
    return StrToWstr(coder.DecodedMessage());
  }
}

std::wstring Base64Decode(const std::wstring& str, bool for_filename) {
  return Base64Decode(WstrToStr(str), for_filename);
}

std::wstring Base64Encode(const std::string& str, bool for_filename) {
  if (str.empty())
    return EmptyString();

  Base64Coder coder;
  coder.Encode((BYTE*)str.c_str(), str.size());

  if (for_filename) {
    std::wstring msg = StrToWstr(coder.EncodedMessage());
    ReplaceChar(msg, '/', '-');
    return msg;
  } else {
    return StrToWstr(coder.EncodedMessage());
  }
}

std::wstring Base64Encode(const std::wstring& str, bool for_filename) {
  return Base64Encode(WstrToStr(str), for_filename);
}