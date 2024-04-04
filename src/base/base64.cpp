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

#include <string>

#include <base64/base64.h>

#include "base/base64.h"

#include "base/string.h"

std::string Base64Decode(const std::string& str) {
  if (str.empty())
    return str;

  Base64Coder coder;
  coder.Decode((BYTE*)str.c_str(), (DWORD)str.size());

  return coder.DecodedMessage();
}

std::string Base64Encode(const std::string& str) {
  if (str.empty())
    return str;

  Base64Coder coder;
  coder.Encode((BYTE*)str.c_str(), (DWORD)str.size());

  return coder.EncodedMessage();
}

////////////////////////////////////////////////////////////////////////////////

std::wstring Base64Decode(const std::wstring& str, bool for_filename) {
  std::wstring output = StrToWstr(Base64Decode(WstrToStr(str)));

  if (for_filename)
    ReplaceChar(output, '-', '/');

  return output;
}

std::wstring Base64Encode(const std::wstring& str, bool for_filename) {
  std::wstring output = StrToWstr(Base64Encode(WstrToStr(str)));

  if (for_filename)
    ReplaceChar(output, '/', '-');

  return output;
}