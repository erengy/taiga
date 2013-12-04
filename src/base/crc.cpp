/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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
#include "crc.h"
#include "string.h"
#include "third_party/zlib/zlib.h"

std::wstring CalculateCrcFromFile(const std::wstring& file) {
  BYTE buffer[0x10000];
  DWORD dwBytesRead = 0;

  HANDLE hFile = CreateFile(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
  if (hFile == INVALID_HANDLE_VALUE)
    return L"";

  ULONG crc = crc32(0L, Z_NULL, 0);
  BOOL bSuccess = ReadFile(hFile, buffer, sizeof(buffer), &dwBytesRead, NULL);
  while (bSuccess && dwBytesRead) {
    crc = crc32(crc, buffer, dwBytesRead);
    bSuccess = ReadFile(hFile, buffer, sizeof(buffer), &dwBytesRead, NULL);
  }

  if (hFile != NULL)
    CloseHandle(hFile);

  wchar_t val[16] = {0};
  _ultow_s(crc, val, 16, 16);
  std::wstring value = val;
  if (value.length() < 8) {
    value.insert(0, 8 - value.length(), '0');
  }

  return value;
}

std::wstring CalculateCrcFromString(const std::wstring& str) {
  std::string text = ToANSI(str);

  ULONG crc = crc32(0L, Z_NULL, 0);
  crc = crc32(crc, (BYTE*)text.data(), text.size());

  wchar_t crc_val[16] = {0};
  _ultow_s(crc, crc_val, 16, 16);
  wstring value = crc_val;
  if (value.length() < 8) {
    value.insert(0, 8 - value.length(), '0');
  }
  ToUpper(value);

  return value;
}