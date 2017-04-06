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

#include "base/string.h"
#include "compat/crypto.h"

namespace compat {

static const wchar_t encryption_char = L'?';
static const wchar_t* encryption_key = L"Tenori Taiga";
static const size_t encryption_length_min = 16;

static std::wstring Xor(std::wstring str, const std::wstring& key) {
  for (size_t i = 0; i < str.size(); i++)
    str[i] = str[i] ^ key[i % key.length()];

  return str;
}

std::wstring SimpleEncrypt(std::wstring str) {
  // Set minimum length
  if (str.length() > 0 && str.length() < encryption_length_min)
    str.append(encryption_length_min - str.length(),
               encryption_char);

  // Encrypt
  str = Xor(str, encryption_key);

  // Convert to hexadecimal string
  std::wstring buffer;
  for (size_t i = 0; i < str.size(); i++) {
    wchar_t c[32] = {'\0'};
    _itow_s(str[i], c, 32, 16);
    if (wcslen(c) == 1)
      buffer.push_back('0');
    buffer += c;
  }
  ToUpper(buffer);

  return buffer;
}

std::wstring SimpleDecrypt(std::wstring str) {
  // Convert from hexadecimal string
  std::wstring buffer;
  for (size_t i = 0; i < str.size(); i = i + 2) {
    wchar_t c = static_cast<wchar_t>(wcstoul(str.substr(i, 2).c_str(),
                                             nullptr, 16));
    buffer.push_back(c);
  }

  // Decrypt
  buffer = Xor(buffer, encryption_key);

  // Trim characters appended to match the minimum length
  if (buffer.size() >= encryption_length_min)
    TrimRight(buffer, std::wstring(1, encryption_char).c_str());

  return buffer;
}

}  // namespace compat
