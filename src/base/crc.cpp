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

#include <windows.h>

#include <zlib/zlib.h>

#include "crc.h"
#include "string.h"

std::wstring ConvertCrcValueToString(ULONG crc) {
  wchar_t crc_val[16] = {0};
  _ultow_s(crc, crc_val, 16, 16);

  std::wstring value = crc_val;

  if (value.length() < 8)
    value.insert(0, 8 - value.length(), '0');

  ToUpper(value);

  return value;
}

std::wstring CalculateCrcFromFile(const std::wstring& file) {
  BYTE buffer[0x10000];
  DWORD bytes_read = 0;

  HANDLE file_handle = CreateFile(
      file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
      OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);

  if (file_handle == INVALID_HANDLE_VALUE)
    return std::wstring();

  ULONG crc = crc32(0L, Z_NULL, 0);
  BOOL success = ReadFile(file_handle, buffer, sizeof(buffer),
                          &bytes_read, nullptr);
  while (success && bytes_read) {
    crc = crc32(crc, buffer, bytes_read);
    success = ReadFile(file_handle, buffer, sizeof(buffer),
                       &bytes_read, nullptr);
  }

  if (file_handle != nullptr)
    CloseHandle(file_handle);

  return ConvertCrcValueToString(crc);
}

std::wstring CalculateCrcFromString(const std::wstring& str) {
  std::string text = WstrToStr(str);

  ULONG crc = crc32(0L, Z_NULL, 0);
  crc = crc32(crc, reinterpret_cast<const Bytef*>(text.data()), text.size());

  return ConvertCrcValueToString(crc);
}