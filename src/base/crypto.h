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

#pragma once

#include <string>

class StringCoder {
public:
  StringCoder();

  bool Encode(const std::wstring& metadata, const std::wstring& data,
              std::wstring& output);
  bool Decode(const std::wstring& input, std::wstring& metadata,
              std::wstring& data);

private:
  std::string ConvertSizeToString(unsigned short value);
  unsigned short ReadSize(const std::string& str, size_t pos);

  const std::string magic_string_;
  const size_t min_length_;
  const unsigned char version_;
};

std::string HmacSha1(const std::string& key_bytes, const std::string& data);
