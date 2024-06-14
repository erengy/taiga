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

#include <algorithm>
#include <iomanip>
#include <locale>
#include <sstream>

#include "base/string.h"

using std::string;
using std::vector;
using std::wstring;

////////////////////////////////////////////////////////////////////////////////
// Erasing

void Erase(wstring& str1, const wstring& str2, bool case_insensitive) {
  if (str2.empty() || str1.length() < str2.length())
    return;

  auto it = str1.begin();

  do {
    if (case_insensitive) {
      it = std::search(it, str1.end(), str2.begin(), str2.end(), &IsCharsEqual);
    } else {
      it = std::search(it, str1.end(), str2.begin(), str2.end());
    }

    if (it != str1.end())
      str1.erase(it, it + str2.length());

  } while (it != str1.end());
}

void EraseChars(wstring& str, const wchar_t chars[]) {
  size_t pos = 0;

  do {
    pos = str.find_first_of(chars, pos);

    if (pos != wstring::npos)
      str.erase(pos, 1);

  } while (pos != wstring::npos);
}

void EraseLeft(wstring& str1, const wstring& str2, bool case_insensitive) {
  if (str1.length() < str2.length())
    return;

  if (case_insensitive) {
    if (!std::equal(str2.begin(), str2.end(), str1.begin(), &IsCharsEqual))
      return;
  } else {
    if (!std::equal(str2.begin(), str2.end(), str1.begin()))
      return;
  }

  str1.erase(str1.begin(), str1.begin() + str2.length());
}

void EraseRight(wstring& str1, const wstring& str2, bool case_insensitive) {
  if (str1.length() < str2.length())
    return;

  if (case_insensitive) {
    if (!std::equal(str2.begin(), str2.end(), str1.end() - str2.length(),
                    &IsCharsEqual))
      return;
  } else {
    if (!std::equal(str2.begin(), str2.end(), str1.end() - str2.length()))
      return;
  }

  str1.resize(str1.length() - str2.length());
}

void RemoveEmptyStrings(vector<wstring>& input) {
  if (input.empty())
    return;

  input.erase(std::remove_if(input.begin(), input.end(),
      [](const wstring& s) -> bool { return s.empty(); }),
      input.end());
}

////////////////////////////////////////////////////////////////////////////////
// Searching and comparison

int CompareStrings(const wstring& str1, const wstring& str2,
                   bool case_insensitive, size_t max_count) {
  if (case_insensitive) {
    return _wcsnicmp(str1.c_str(), str2.c_str(), max_count);
  } else {
    return wcsncmp(str1.c_str(), str2.c_str(), max_count);
  }
}

int InStr(const wstring& str1, const wstring& str2, int pos,
          bool case_insensitive) {
  if (str1.empty())
    return -1;
  if (str2.empty())
    return 0;
  if (str1.length() < str2.length())
    return -1;

  if (case_insensitive) {
    auto i = std::search(str1.begin() + pos, str1.end(),
                         str2.begin(), str2.end(),
                         &IsCharsEqual);
    return (i == str1.end()) ? -1 : static_cast<int>(i - str1.begin());
  } else {
    const size_t i = str1.find(str2, pos);
    return (i != wstring::npos) ? static_cast<int>(i) : -1;
  }
}

wstring InStr(const wstring& str1, const wstring& str2_left,
              const wstring& str2_right) {
  wstring output;

  int index_begin = InStr(str1, str2_left);

  if (index_begin > -1) {
    index_begin += static_cast<int>(str2_left.length());
    int index_end = InStr(str1, str2_right, index_begin);
    if (index_end > -1)
      output = str1.substr(index_begin, index_end - index_begin);
  }

  return output;
}

int InStrChars(const wstring& str1, const wstring& str2, int pos) {
  const size_t i = str1.find_first_of(str2, pos);
  return (i != wstring::npos) ? static_cast<int>(i) : -1;
}

bool IsAlphanumericChar(const wchar_t c) {
  return (c >= L'0' && c <= L'9') ||
         (c >= L'A' && c <= L'Z') ||
         (c >= L'a' && c <= L'z');
}
bool IsAlphanumericString(const wstring& str) {
  return !str.empty() &&
         std::all_of(str.begin(), str.end(), IsAlphanumericChar);
}

inline bool IsCharsEqual(const wchar_t c1, const wchar_t c2) {
  return tolower(c1) == tolower(c2);
}

bool IsEqual(const wstring& str1, const wstring& str2) {
  if (str1.length() != str2.length())
    return false;

  return std::equal(str1.begin(), str1.end(), str2.begin(), &IsCharsEqual);
}

bool IsHexadecimalChar(const wchar_t c) {
  return (c >= L'0' && c <= L'9') ||
         (c >= L'A' && c <= L'F') ||
         (c >= L'a' && c <= L'f');
}
bool IsHexadecimalString(const wstring& str) {
  return !str.empty() &&
         std::all_of(str.begin(), str.end(), IsHexadecimalChar);
}

bool IsNumericChar(const wchar_t c) {
  return c >= L'0' && c <= L'9';
}
bool IsNumericString(const wstring& str) {
  return !str.empty() &&
         std::all_of(str.begin(), str.end(), IsNumericChar);
}

bool StartsWith(const wstring& str1, const wstring& str2) {
  return str1.compare(0, str2.length(), str2) == 0;
}

bool EndsWith(const wstring& str1, const wstring& str2) {
  if (str2.length() > str1.length())
    return false;

  return str1.compare(str1.length() - str2.length(), str2.length(), str2) == 0;
}

////////////////////////////////////////////////////////////////////////////////

size_t LongestCommonSubsequenceLength(const wstring& str1,
                                      const wstring& str2) {
  if (str1.empty() || str2.empty())
    return 0;

  const size_t len1 = str1.length();
  const size_t len2 = str2.length();

  vector<vector<size_t>> table(len1 + 1);
  for (auto it = table.begin(); it != table.end(); ++it)
    it->resize(len2 + 1);

  for (size_t i = 0; i < len1; i++) {
    for (size_t j = 0; j < len2; j++) {
      if (str1[i] == str2[j]) {
        table[i + 1][j + 1] = table[i][j] + 1;
      } else {
        table[i + 1][j + 1] = std::max(table[i + 1][j], table[i][j + 1]);
      }
    }
  }

  return table.back().back();
}

////////////////////////////////////////////////////////////////////////////////

// Based on Miguel Serrano's Jaro-Winkler distance implementation
// Licensed under GNU GPLv3 - Copyright (C) 2011 Miguel Serrano
double JaroWinklerDistance(const wstring& str1, const wstring& str2) {
  const int len1 = static_cast<int>(str1.size());
  const int len2 = static_cast<int>(str2.size());

  if (!len1 || !len2)
    return 0.0;

  int i, j, l;
  int m = 0, t = 0;
  vector<int> sflags(len1), aflags(len2);

  // Calculate matching characters
  int range = std::max(0, (std::max(len1, len2) / 2) - 1);
  for (i = 0; i < len2; i++) {
    for (j = std::max(i - range, 0), l = std::min(i + range + 1, len1); j < l; j++) {
      if (str2[i] == str1[j] && !sflags[j]) {
        sflags[j] = 1;
        aflags[i] = 1;
        m++;
        break;
      }
    }
  }
  if (!m)
    return 0.0;

  // Calculate character transpositions
  l = 0;
  for (i = 0; i < len2; i++) {
    if (aflags[i] == 1) {
      for (j = l; j < len1; j++) {
        if (sflags[j] == 1) {
          l = j + 1;
          break;
        }
      }
      if (str2[i] != str1[j])
        t++;
    }
  }
  t /= 2;

  // Jaro distance
  double dw = ((static_cast<double>(m) / len1) +
               (static_cast<double>(m) / len2) +
               (static_cast<double>(m - t) / m)) / 3.0;

  // Calculate common string prefix up to 4 chars
  l = 0;
  for (i = 0; i < std::min(std::min(len1, len2), 4); i++)
    if (str1[i] == str2[i])
        l++;

  // Jaro-Winkler distance
  const double scaling_factor = 0.1;
  dw = dw + (l * scaling_factor * (1.0 - dw));

  return dw;
}

double LevenshteinDistance(const wstring& str1, const wstring& str2) {
  const size_t len1 = str1.size();
  const size_t len2 = str2.size();

  vector<size_t> prev_col(len2 + 1);
  for (size_t i = 0; i < prev_col.size(); i++)
    prev_col[i] = i;

  vector<size_t> col(len2 + 1);

  for (size_t i = 0; i < len1; i++) {
    col[0] = i + 1;

    for (size_t j = 0; j < len2; j++)
      col[j + 1] = std::min(std::min(1 + col[j], 1 + prev_col[1 + j]),
                            prev_col[j] + (str1[i] == str2[j] ? 0 : 1));

    col.swap(prev_col);
  }

  const double len = static_cast<double>(std::max(str1.size(), str2.size()));
  return 1.0 - (prev_col[len2] / len);
}

////////////////////////////////////////////////////////////////////////////////

void GetTrigrams(const wstring& str, trigram_container_t& output) {
  const size_t n = 3;

  if (n < 1)
    return;

  output.clear();

  if (n >= str.size()) {
    trigram_t buffer = {'\0'};
    std::copy(str.begin(), str.end(), buffer.begin());
    output.push_back(buffer);
    return;
  }

  for (size_t i = 0; i <= str.size() - n; ++i) {
    trigram_t buffer = {'\0'};
    std::copy(str.begin() + i, str.begin() + (i + n), buffer.begin());
    output.push_back(buffer);
  }

  std::sort(output.begin(), output.end());
}

double CompareTrigrams(const trigram_container_t& t1,
                       const trigram_container_t& t2) {
  trigram_container_t intersection;

  std::set_intersection(t1.begin(), t1.end(), t2.begin(), t2.end(),
                        std::back_inserter(intersection));

  return static_cast<double>(intersection.size()) /
         static_cast<double>(std::max(t1.size(), t2.size()));
}

////////////////////////////////////////////////////////////////////////////////
// Replace

void ReplaceChar(wstring& str, const wchar_t c, const wchar_t replace_with) {
  if (c == replace_with)
    return;

  size_t pos = 0;

  do {
    pos = str.find_first_of(c, pos);
    if (pos != wstring::npos)
      str.at(pos) = replace_with;
  } while (pos != wstring::npos);
}

bool ReplaceString(wstring& str,
                   size_t offset,
                   const wstring& find_this,
                   const wstring& replace_with,
                   bool whole_word_only,
                   bool replace_all_instances) {
  if (find_this.empty() || find_this == replace_with ||
      str.length() < find_this.length() || offset >= str.length())
    return false;

  bool found_and_replaced = false;

  for (size_t pos = str.find(find_this, offset);
       pos != wstring::npos; pos = str.find(find_this, pos)) {

    auto is_whole_word = [&]() {
      auto is_boundary = [](wchar_t c) {
        return iswspace(c) || iswpunct(c);
      };
      if (pos == 0 || is_boundary(str.at(pos - 1))) {
        size_t pos_end = pos + find_this.length();
        if (pos_end >= str.length() || is_boundary(str.at(pos_end)))
          return true;
      }
      return false;
    };

    if (whole_word_only && !is_whole_word()) {
      pos += find_this.length();
    } else {
      str.replace(pos, find_this.length(), replace_with);
      pos += replace_with.length();
      found_and_replaced = true;
      if (!replace_all_instances)
        break;
    }
  }

  return found_and_replaced;
}

bool ReplaceString(wstring& str,
                   const wstring& find_this,
                   const wstring& replace_with) {
  return ReplaceString(str, 0, find_this, replace_with, false, true);
}

////////////////////////////////////////////////////////////////////////////////
// Split, tokenize

wstring Join(const vector<wstring>& join_vector, const wstring& separator) {
  wstring str;

  for (auto it = join_vector.begin(); it != join_vector.end(); ++it) {
    if (it != join_vector.begin())
      str += separator;
    str += *it;
  }

  return str;
}

void Split(const wstring& str, const wstring& separator,
           vector<wstring>& split_vector) {
  if (separator.empty()) {
    split_vector.push_back(str);
    return;
  }

  size_t index_begin = 0, index_end;

  do {
    index_end = str.find(separator, index_begin);
    if (index_end == wstring::npos)
      index_end = str.length();

    split_vector.push_back(str.substr(index_begin, index_end - index_begin));

    index_begin = index_end + separator.length();
  } while (index_begin <= str.length());
}

size_t Tokenize(const wstring& str, const wstring& delimiters,
                vector<wstring>& tokens) {
  tokens.clear();

  size_t index_begin = str.find_first_not_of(delimiters);

  while (index_begin != wstring::npos) {
    size_t index_end = str.find_first_of(delimiters, index_begin + 1);
    if (index_end == wstring::npos) {
      tokens.push_back(str.substr(index_begin));
      break;
    } else {
      tokens.push_back(str.substr(index_begin, index_end - index_begin));
      index_begin = str.find_first_not_of(delimiters, index_end + 1);
    }
  }

  return tokens.size();
}

////////////////////////////////////////////////////////////////////////////////
// std::string <-> std::wstring conversion

wstring StrToWstr(const string& str, UINT code_page) {
  if (!str.empty()) {
    int length = MultiByteToWideChar(code_page, 0, str.c_str(), -1, nullptr, 0);
    if (length > 0) {
      vector<wchar_t> output(length);
      MultiByteToWideChar(code_page, 0, str.c_str(), -1, &output[0], length);
      return &output[0];
    }
  }

  return wstring();
}

string WstrToStr(const wstring& str, UINT code_page) {
  if (!str.empty()) {
    int length = WideCharToMultiByte(code_page, 0, str.c_str(), -1, nullptr, 0,
                                     nullptr, nullptr);
    if (length > 0) {
      vector<char> output(length);
      WideCharToMultiByte(code_page, 0, str.c_str(), -1, &output[0], length,
                          nullptr, nullptr);
      return &output[0];
    }
  }

  return string();
}

////////////////////////////////////////////////////////////////////////////////
// Case conversion

// System-default ANSI code page
std::locale current_locale("");

void ToLower(wstring& str, bool use_locale) {
  if (use_locale) {
    std::transform(str.begin(), str.end(), str.begin(),
        [](const wchar_t c) {
          return std::tolower(c, current_locale);
        });
  } else {
    std::transform(str.begin(), str.end(), str.begin(), towlower);
  }
}

wstring ToLower_Copy(wstring str, bool use_locale) {
  ToLower(str, use_locale);
  return str;
}

void ToUpper(wstring& str, bool use_locale) {
  if (use_locale) {
    std::transform(str.begin(), str.end(), str.begin(),
        [](const wchar_t c) {
          return std::toupper(c, current_locale);
        });
  } else {
    std::transform(str.begin(), str.end(), str.begin(), towupper);
  }
}

wstring ToUpper_Copy(wstring str, bool use_locale) {
  ToUpper(str, use_locale);
  return str;
}

////////////////////////////////////////////////////////////////////////////////
// Type conversion

bool ToBool(const wstring& str) {
  if (str.empty())
    return false;

  const wchar_t c = str.front();

  return (c == '1' || c == 't' || c == 'T' || c == 'y' || c == 'Y');
}

double ToDouble(const string& str) {
  return atof(str.c_str());
}

double ToDouble(const wstring& str) {
  return _wtof(str.c_str());
}

int ToInt(const string& str) {
  return atoi(str.c_str());
}

int ToInt(const wstring& str) {
  return _wtoi(str.c_str());
}

UINT64 ToUint64(const std::string& str) {
  return std::stoull(str.c_str(), nullptr, 10);
}

UINT64 ToUint64(const std::wstring& str) {
  return std::wcstoull(str.c_str(), nullptr, 10);
}

time_t ToTime(const std::string& str) {
  return _atoi64(str.c_str());
}

time_t ToTime(const std::wstring& str) {
  return _wtoi64(str.c_str());
}

string ToStr(const INT& value) {
  char buffer[65];
  _ltoa_s(value, buffer, 65, 10);
  return string(buffer);
}

wstring ToWstr(const INT& value) {
  wchar_t buffer[65];
  _ltow_s(value, buffer, 65, 10);
  return wstring(buffer);
}

wstring ToWstr(const UINT& value) {
  wchar_t buffer[65];
  _ultow_s(value, buffer, 65, 10);
  return wstring(buffer);
}

wstring ToWstr(const ULONG& value) {
  wchar_t buffer[65];
  _ultow_s(value, buffer, 65, 10);
  return wstring(buffer);
}

wstring ToWstr(const INT64& value) {
  wchar_t buffer[65];
  _i64tow_s(value, buffer, 65, 10);
  return wstring(buffer);
}

wstring ToWstr(const UINT64& value) {
  wchar_t buffer[65];
  _ui64tow_s(value, buffer, 65, 10);
  return wstring(buffer);
}

string ToStr(const double& value, int count) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(count) << value;
  return out.str();
}

wstring ToWstr(const double& value, int count) {
  std::wostringstream out;
  out << std::fixed << std::setprecision(count) << value;
  return out.str();
}

////////////////////////////////////////////////////////////////////////////////
// Trimming

wstring LimitText(const wstring& str, unsigned int limit, const wstring& tail) {
  if (str.length() > limit) {
    wstring limit_str = str.substr(0, limit);
    if (!tail.empty() && limit_str.length() > tail.length())
      limit_str.replace(limit_str.length() - tail.length(),
                        tail.length(), tail);
    return limit_str;
  } else {
    return str;
  }
}

void Trim(wstring& str, const wchar_t trim_chars[],
          bool trim_left, bool trim_right) {
  if (str.empty())
    return;

  const size_t index_begin =
      trim_left ? str.find_first_not_of(trim_chars) : 0;
  const size_t index_end =
      trim_right ? str.find_last_not_of(trim_chars) : str.length() - 1;

  if (index_begin == wstring::npos || index_end == wstring::npos) {
    str.clear();
    return;
  }

  if (trim_right)
    str.erase(index_end + 1, str.length() - index_end + 1);
  if (trim_left)
    str.erase(0, index_begin);
}

void TrimLeft(wstring& str, const wchar_t trim_chars[]) {
  Trim(str, trim_chars, true, false);
}

void TrimRight(wstring& str, const wchar_t trim_chars[]) {
  Trim(str, trim_chars, false, true);
}

////////////////////////////////////////////////////////////////////////////////
// File and folder related

void AddTrailingSlash(wstring& str) {
  if (str.length() > 0 && str[str.length() - 1] != '\\')
    str += L"\\";
}

wstring AddTrailingSlash(const wstring& str) {
  if (str.length() > 0 && str[str.length() - 1] != '\\') {
    return str + L"\\";
  } else {
    return str;
  }
}

void RemoveTrailingSlash(wstring& str) {
  TrimRight(str, L"/\\");
}

wstring GetFileExtension(const wstring& str) {
  return str.substr(str.find_last_of(L".") + 1);
}

wstring GetFileWithoutExtension(wstring str) {
  size_t pos = str.find_last_of(L".");

  if (pos != wstring::npos)
    str.resize(pos);

  return str;
}

wstring GetFileName(const wstring& str) {
  return str.substr(str.find_last_of(L"/\\") + 1);
}

wstring GetPathOnly(const wstring& str) {
  return str.substr(0, str.find_last_of(L"/\\") + 1);
}

bool ValidateFileExtension(const wstring& extension, unsigned int max_length) {
  if (max_length > 0 && extension.length() > max_length)
    return false;

  if (!IsAlphanumericString(extension))
    return false;

  return true;
}

////////////////////////////////////////////////////////////////////////////////
// Other

void AppendString(wstring& str0, const wstring& str1, const wstring& str2) {
  if (str1.empty())
    return;

  if (!str0.empty())
    str0.append(str2);

  str0.append(str1);
}

const wstring& EmptyString() {
  static const wstring str;
  return str;
}

wstring PadChar(wstring str, const wchar_t ch, const size_t len) {
  if (len > str.length())
    str.insert(0, len - str.length(), ch);

  return str;
}

wstring PushString(const wstring& str1, const wstring& str2) {
  if (str2.empty()) {
    return L"";
  } else {
    return str1 + str2;
  }
}
