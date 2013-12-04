/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#include "common.h"
#include "string.h"
#include "library/anime.h"
#include "third_party/zlib/zlib.h"
#include "ui/theme.h"

// =============================================================================

wstring CalculateCRC(const wstring& file) {
  BYTE buffer[0x10000];
  DWORD dwBytesRead = 0;
  
  HANDLE hFile = CreateFile(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
  if (hFile == INVALID_HANDLE_VALUE) return L"";

  ULONG crc = crc32(0L, Z_NULL, 0);
  BOOL bSuccess = ReadFile(hFile, buffer, sizeof(buffer), &dwBytesRead, NULL);
  while (bSuccess && dwBytesRead) {
    crc = crc32(crc, buffer, dwBytesRead);
    bSuccess = ReadFile(hFile, buffer, sizeof(buffer), &dwBytesRead, NULL);
  }

  if (hFile != NULL) CloseHandle(hFile);

  wchar_t val[16] = {0};
  _ultow_s(crc, val, 16, 16);
  wstring value = val;
  if (value.length() < 8) {
    value.insert(0, 8 - value.length(), '0');
  }
  return value;
}

// =============================================================================

int GetEpisodeHigh(const wstring& episode_number) {
  int value = 1, pos = InStrRev(episode_number, L"-", episode_number.length());
  if (pos == episode_number.length() - 1) {
    value = ToInt(episode_number.substr(0, pos));
  } else if (pos > -1) {
    value = ToInt(episode_number.substr(pos + 1));
  } else {
    value = ToInt(episode_number);
  }
  return value;
}

int GetEpisodeLow(const wstring& episode_number) {
  return ToInt(episode_number); // ToInt() stops at -
}

void SplitEpisodeNumbers(const wstring& input, vector<int>& output) {
  if (input.empty()) return;
  vector<wstring> numbers;
  Split(input, L"-", numbers);
  for (auto it = numbers.begin(); it != numbers.end(); ++it) {
    output.push_back(ToInt(*it));
  }
}

wstring JoinEpisodeNumbers(const vector<int>& input) {
  wstring output;
  for (auto it = input.begin(); it != input.end(); ++it) {
    if (!output.empty()) output += L"-";
    output += ToWstr(*it);
  }
  return output;
}

int TranslateResolution(const wstring& str, bool return_validity) {
  // *###x###*
  if (str.length() > 6) {
    int pos = InStr(str, L"x", 0);
    if (pos > -1) {
      for (unsigned int i = 0; i < str.length(); i++) {
        if (i != pos && !IsNumeric(str.at(i))) return 0;
      }
      return return_validity ? 
        TRUE : ToInt(str.substr(pos + 1));
    }

  // *###p
  } else if (str.length() > 3) {
    if (str.at(str.length() - 1) == 'p') {
      for (unsigned int i = 0; i < str.length() - 1; i++) {
        if (!IsNumeric(str.at(i))) return 0;
      }
      return return_validity ? 
        TRUE : ToInt(str.substr(0, str.length() - 1));
    }
  }

  return 0;
}

// =============================================================================

int StatusToIcon(int status) {  
  switch (status) {
    case anime::kAiring:
      return ICON16_GREEN;
    case anime::kFinishedAiring:
      return ICON16_BLUE;
    case anime::kNotYetAired:
      return ICON16_RED;
    default:
      return ICON16_GRAY;
  }
}

// =============================================================================

wstring FormatError(DWORD dwError, LPCWSTR lpSource) {
  DWORD dwFlags = FORMAT_MESSAGE_IGNORE_INSERTS;
  HMODULE hInstance = NULL;
  const DWORD size = 101;
  WCHAR buffer[size];

  if (lpSource) {
    dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    hInstance = LoadLibrary(lpSource);
    if (!hInstance) return L"";
  } else {
    dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
  }

  if (FormatMessage(dwFlags, hInstance, dwError, 
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, size, NULL)) {
      if (hInstance) FreeLibrary(hInstance);
      return buffer;
  } else {
    if (hInstance) FreeLibrary(hInstance);
    return ToWstr(dwError);
  }
}

void SetSharedCursor(LPCWSTR name) {
  SetCursor(reinterpret_cast<HCURSOR>(LoadImage(nullptr, name, IMAGE_CURSOR,
                                                0, 0, LR_SHARED)));
}