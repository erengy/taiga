/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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

#include "std.h"
#include "string.h"
#include <sstream>
#include <iomanip>

// =============================================================================

/* Erasing */

void Erase(wstring& input, const wstring& find, bool case_insensitive) {
  if (find.empty() || input.length() < find.length()) return;
  if (!case_insensitive) {
    for (size_t pos = input.find(find); pos != wstring::npos; pos = input.find(find, pos)) {
      input.erase(pos, find.length());
    }
  } else {
    for (size_t i = 0; i < input.length() - find.length() + 1; i++) {
      for (size_t j = 0; j < find.length(); j++) {
        if (input.length() < find.length()) return;
        if (tolower(input[i + j]) == tolower(find[j])) {
          if (j == find.length() - 1) {
            input.erase(i--, find.length());
            break;
          }
        } else {
          i += j;
          break;
        }
      }
    }
  }
}

void EraseChars(wstring& input, const wchar_t chars[]) {
  size_t pos = 0;
  do {
    pos = input.find_first_of(chars, pos);
    if (pos != wstring::npos) input.erase(pos, 1);
  } while (pos != wstring::npos);
}

void ErasePunctuation(wstring& input, bool keep_trailing) {
  wchar_t c;
  for (int i = input.length() - 1; i >= 0; i--) {
    c = input[i];
    if ((c >  31 && c <  48) || // !"#$%&'()*+,-./
        (c >  57 && c <  65) || // :;<=>?@
        (c >  90 && c <  97) || // [\]^_`
        (c > 122 && c < 128)) { // {|}~
          if (keep_trailing) {
            if (c == '!' ||  // "Hayate no Gotoku!", "K-ON!"...
                c == '+' ||  // "Needless+"
                c == '\'') { // "Gintama'"
                  continue;
            }
            keep_trailing = false;
          }
          input.erase(i, 1);
    } else {
      keep_trailing = false;
    }
  }
}

void EraseLeft(wstring& str, const wstring find, bool case_insensitive) {
  if (str.length() < find.length()) return;
  for (size_t i = 0; i < find.length(); i++) {
    if (case_insensitive) {
      if (tolower(str[i]) != tolower(find[i])) return;
    } else {
      if (str[i] != find[i]) return;
    }
  }
  str = str.substr(find.length(), str.length() - find.length());
}

void EraseRight(wstring& str, const wstring find, bool case_insensitive) {
  if (str.length() < find.length()) return;
  for (size_t i = 0; i < find.length(); i++) {
    if (case_insensitive) {
      if (tolower(str[str.length() - find.length() + i]) != tolower(find[i])) return;
    } else {
      if (str[str.length() - find.length() + i] != find[i]) return;
    }
  }
  str.resize(str.length() - find.length());
}

// =============================================================================

/* Searching and comparison */

wstring CharLeft(const wstring& str, int length) {
  return str.substr(0, length);
}

wstring CharRight(const wstring& str, int length) {
  return str.substr(str.length() - length, length);
}

int CompareStrings(const wstring& str1, const wstring& str2, bool case_insensitive, size_t max_count) {
  if (case_insensitive) {
    return _wcsnicmp(str1.c_str(), str2.c_str(), max_count);
  } else {
    return wcsncmp(str1.c_str(), str2.c_str(), max_count);
  }
}

int InStr(const wstring& str, const wstring find, int pos, bool case_insensitive) {
  if (str.empty()) return -1; if (find.empty()) return 0;
  if (str.length() < find.length()) return -1;
  if (!case_insensitive) {
    size_t i = str.find(find, pos);
    return (i != wstring::npos) ? i : -1;
  } else {
    for (size_t i = pos; i < str.length() - find.length() + 1; i++) {
      if (tolower(str[i]) != tolower(find[0])) continue;
      if (_wcsnicmp(str.substr(i).c_str(), find.c_str(), find.length()) == 0)
        return i;
    }
    return -1;
  }
}

wstring InStr(const wstring& str, const wstring& str_left, const wstring& str_right) {
  wstring output;
  int index_begin = InStr(str, str_left);
  if (index_begin > -1) {
    index_begin += str_left.length();
    int index_end = InStr(str, str_right, index_begin);
    if (index_end > -1) {
      output = str.substr(index_begin, index_end - index_begin);
    }
  }
  return output;
}

int InStrRev(const wstring& str, const wstring find, int pos) {
  size_t i = str.rfind(find, pos);
  return (i != wstring::npos) ? i : -1;
}

int InStrChars(const wstring& str, const wstring find, int pos) {
  size_t i = str.find_first_of(find, pos);
  return (i != wstring::npos) ? i : -1;
}

int InStrCharsRev(const wstring& str, const wstring find, int pos) {
  size_t i = str.find_last_of(find, pos);
  return (i != wstring::npos) ? i : -1;
}

bool IsAlphanumeric(const wchar_t c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
bool IsAlphanumeric(const wstring& str) {
  if (str.empty()) return false;
  for (size_t i = 0; i < str.length(); i++)
    if (!IsAlphanumeric(str[i])) return false;
  return true;
}

bool IsEqual(const wstring& str1, const wstring& str2) {
  if (str1.length() != str2.length()) return false;
  return _wcsnicmp(str1.c_str(), str2.c_str(), MAX_PATH) == 0;
}

bool IsHex(const wchar_t c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}
bool IsHex(const wstring& str) {
  if (str.empty()) return false;
  for (size_t i = 0; i < str.length(); i++)
    if (!IsHex(str[i])) return false;
  return true;
}

bool IsNumeric(const wchar_t c) {
  return c >= '0' && c <= '9';
}
bool IsNumeric(const wstring& str) {
  if (str.empty()) return false;
  for (size_t i = 0; i < str.length(); i++)
    if (!IsNumeric(str[i])) return false;
  return true;
}

bool IsWhitespace(const wchar_t c) {
  return c == ' ' || c == '\r' || c == '\n' || c == '\t';
}

bool StartsWith(const wstring& str, const wstring& search) {
  return str.compare(0, search.length(), search) == 0;
}

bool EndsWith(const wstring& str, const wstring& search) {
  if (search.length() > str.length()) return false;
  return str.compare(str.length() - search.length(), search.length(), search) == 0;
}

// =============================================================================

/* Replace */

void Replace(wstring& input, wstring find, wstring replace_with, bool replace_all, bool case_insensitive) {
  if (find.empty() || find == replace_with || input.length() < find.length()) return;
  if (!case_insensitive) {
    for (size_t pos = input.find(find); pos != wstring::npos; pos = input.find(find, pos)) {
      input.replace(pos, find.length(), replace_with);
      if (!replace_all) pos += replace_with.length();
    }
  } else {
    for (size_t i = 0; i < input.length() - find.length() + 1; i++) {
      for (size_t j = 0; j < find.length(); j++) {
        if (input.length() < find.length()) return;
        if (tolower(input[i + j]) == tolower(find[j])) {
          if (j == find.length() - 1) {
            input.replace(i--, find.length(), replace_with);
            if (!replace_all) i += replace_with.length();
            break;
          }
        } else {
          i += j;
          break;
        }
      }
    }
  }
}

void ReplaceChar(wstring& input, const wchar_t c, const wchar_t replace_with) {
  if (c == replace_with) return;
  size_t pos = 0;
  do {
    pos = input.find_first_of(c, pos);
    if (pos != wstring::npos) input.at(pos) = replace_with;
  } while (pos != wstring::npos);
}

void ReplaceChars(wstring& input, const wchar_t chars[], const wstring replace_with) {
  if (chars == replace_with) return;
  size_t pos = 0;
  do {
    pos = input.find_first_of(chars, pos);
    if (pos != wstring::npos) input.replace(pos, 1, replace_with);
  } while (pos != wstring::npos);
}

// =============================================================================

/* Split, tokenize */

void Split(const wstring& input, const wstring& separator, std::vector<wstring>& split_vector) {
  if (separator.empty()) {
    split_vector.push_back(input);
    return;
  }
  size_t index_begin = 0, index_end;
  do {
    index_end = input.find(separator, index_begin);
    if (index_end == wstring::npos) index_end = input.length();
    split_vector.push_back(input.substr(index_begin, index_end - index_begin));
    index_begin = index_end + separator.length();
  } while (index_begin <= input.length());
}

wstring SubStr(const wstring& input, const wstring& sub_begin, const wstring& sub_end) {
  size_t index_begin = input.find(sub_begin, 0);
  if (index_begin == wstring::npos) return L"";
  size_t index_end = input.find(sub_end, index_begin);
  if (index_end == wstring::npos) index_end = input.length();
  return input.substr(index_begin + 1, index_end - index_begin - 1);
}

size_t Tokenize(const wstring& input, const wstring& delimiters, vector<wstring>& tokens) {
  tokens.clear();
  size_t index_begin = input.find_first_not_of(delimiters);
  while (index_begin != wstring::npos) {
    size_t index_end = input.find_first_of(delimiters, index_begin + 1);
    if (index_end == wstring::npos) {
      tokens.push_back(input.substr(index_begin));
      break;
    } else {
      tokens.push_back(input.substr(index_begin, index_end - index_begin));
      index_begin = input.find_first_not_of(delimiters, index_end + 1);
    }
  }
  return tokens.size();
}

// =============================================================================

/* ANSI <-> Unicode conversion */

const char* ToANSI(const wstring& input, UINT code_page) {
  if (input.empty()) return "";
  int length = WideCharToMultiByte(code_page, 0, input.c_str(), -1, NULL, 0, NULL, NULL);
  if (length > 0) {
    char* output = new char[length + 1];
    WideCharToMultiByte(code_page, 0, input.c_str(), -1, output, length, NULL, NULL);
    return output;
  } else {
    return "";
  }
}

wstring ToUTF8(const string& input, UINT code_page) {
  if (input.empty()) return L"";
  int length = MultiByteToWideChar(code_page, 0, input.c_str(), -1, NULL, 0);
  if (length > 0) {
    vector<wchar_t> output(length);
    MultiByteToWideChar(code_page, 0, input.c_str(), -1, &output[0], length);
    return &output[0];
  } else {
    return L"";
  }
}

// =============================================================================

/* Case conversion */

void ToLower(wstring& str) {
  for (size_t i = 0; i < str.length(); i++)
    str[i] = tolower(str[i]);
}

wstring ToLower_Copy(wstring str) {
  for (size_t i = 0; i < str.length(); i++)
    str[i] = tolower(str[i]);
  return str;
}

void ToUpper(wstring& str) {
  for (size_t i = 0; i < str.length(); i++)
    str[i] = toupper(str[i]);
}

wstring ToUpper_Copy(wstring str) {
  for (size_t i = 0; i < str.length(); i++)
    str[i] = toupper(str[i]);
  return str;
}

// =============================================================================

/* Type conversion */

int ToINT(const wstring& str) {
  return _wtoi(str.c_str());
}

wstring ToWSTR(const INT& value) {
  wchar_t buffer[65];
  _ltow_s(value, buffer, 65, 10);
  return wstring(buffer);
}

wstring ToWSTR(const ULONG& value) {
  wchar_t buffer[65];
  _ultow_s(value, buffer, 65, 10);
  return wstring(buffer);
}

wstring ToWSTR(const INT64& value) {
  wchar_t buffer[65];
  _i64tow_s(value, buffer, 65, 10);
  return wstring(buffer);
}

wstring ToWSTR(const UINT64& value) {
  wchar_t buffer[65];
  _ui64tow_s(value, buffer, 65, 10);
  return wstring(buffer);
}

wstring ToWSTR(const double& value, int count) {
  std::wostringstream out;
  out << std::fixed << std::setprecision(count) << value;
  return out.str();
}

// =============================================================================

/* Encoding & Decoding */

wstring EncodeURL(const wstring& str, bool encode_unreserved) {
  static const wchar_t* digits = L"0123456789ABCDEF";
  wstring output;

  for (unsigned int i = 0; i < str.length(); i++) {
    if ((str[i] >= '0' && str[i] <= '9') || 
        (str[i] >= 'A' && str[i] <= 'Z') || 
        (str[i] >= 'a' && str[i] <= 'z') || 
        (!encode_unreserved && 
        (str[i] == '-' || str[i] == '.' || 
         str[i] == '_' || str[i] == '~'))) {
           output.push_back(str[i]);
    } else {
      #define PercentEncode(x) \
        output.append(L"%"); \
        output.append(&digits[(x >> 4) & 0x0F], 1); \
        output.append(&digits[x & 0x0F], 1);
      if (str[i] > 255) {
        string buffer = ToANSI(wstring(&str[i], 1));
        for (unsigned int j = 0; j < buffer.length(); j++) {
          PercentEncode(buffer[j]);
        }
      } else {
        PercentEncode(str[i]);
      }
      #undef PercentEncode
    }
  }

  return output;
}

void DecodeHTML(wstring& input) {
  #define HTMLCHARCOUNT 28
  static const wchar_t* html_chars[HTMLCHARCOUNT][2] = {
    {L"&quot;",   L"\""},     // quotation mark
    {L"&amp;",    L"&"},      // ampersand
    {L"&apos;",   L"'"},      // apostrophe
    {L"&#039;",   L"'"},      // apostrophe
    {L"&lt;",     L"<"},      // less-than
    {L"&gt;",     L">"},      // greater-than
    {L"&nbsp;",   L" "},      // non-breaking space
    {L"&yen;",    L"\u00A5"}, // yen
    {L"&copy;",   L"\u00A9"}, // copyright
    {L"&laquo;",  L"\u00AB"}, // angle quotation mark (left)
    {L"&reg;",    L"\u00AE"}, // registered trademark
    {L"&deg;",    L"\u00B0"}, // degree
    {L"&acute;",  L"\u00B4"}, // spacing acute
    {L"&raquo;",  L"\u00BB"}, // angle quotation mark (right)
    {L"&Egrave;", L"\u00C8"}, // capital e, grave accent
    {L"&Eacute;", L"\u00C9"}, // capital e, acute accent
    {L"&egrave;", L"\u00E8"}, // small e, grave accent
    {L"&eacute;", L"\u00E9"}, // small e, acute accent
	{L"&auml;",   L"\u00E4"}, // small a, diaeresis
    {L"&ndash;",  L"\u2013"}, // en dash
    {L"&mdash;",  L"\u2014"}, // em dash
    {L"&lsquo;",  L"\u2018"}, // left single quotation mark
    {L"&rsquo;",  L"\u2019"}, // right single quotation mark
    {L"&ldquo;",  L"\u201C"}, // left double quotation mark
    {L"&rdquo;",  L"\u201D"}, // right double quotation mark
    {L"&dagger;", L"\u2020"}, // dagger
    {L"&hellip;", L"\u2026"}, // horizontal ellipsis
    {L"&trade;",  L"\u2122"}  // trademark
  };
  if (InStr(input, L"&") > -1) {
    for (int i = 0; i < HTMLCHARCOUNT; i++) {
      Replace(input, html_chars[i][0], html_chars[i][1], true, false);
    }
  }
  #undef HTMLCHARCOUNT
}

void StripHTML(wstring& str) {
  int index_begin, index_end;
  do {
    index_begin = InStr(str, L"<", 0);
    if (index_begin > -1) {
      index_end = InStr(str, L">", index_begin);
      if (index_end > -1) {
        str.erase(index_begin, index_end - index_begin + 1);
      } else {
        break;
      }
    }
  } while (index_begin > -1);
}

// =============================================================================

/* Trimming */

void LimitText(wstring& input, unsigned int limit, const wstring& tail) {
  if (input.length() > limit) {
    input.resize(limit);
    if (!tail.empty() && input.length() > tail.length()) {
      input.replace(input.length() - tail.length(), tail.length(), tail);
    }
  }
}

void Trim(wstring& input, const wchar_t trim_chars[], bool trim_left, bool trim_right) {
  if (input.empty()) return;
  const size_t index_begin = trim_left ? input.find_first_not_of(trim_chars) : 0;
  const size_t index_end = trim_right ? input.find_last_not_of(trim_chars) : input.length() - 1;
  if (index_begin == wstring::npos || index_end == wstring::npos) return;
  if (trim_right) input.erase(index_end + 1, input.length() - index_end + 1);
  if (trim_left) input.erase(0, index_begin);
}

void TrimLeft(wstring& input, const wchar_t trim_chars[]) {
  Trim(input, trim_chars, true, false);
}

void TrimRight(wstring& input, const wchar_t trim_chars[]) {
  Trim(input, trim_chars, false, true);
}

// =============================================================================

/* File and folder related */

bool CheckFileExtension(wstring extension, const vector<wstring>& extension_list) {
  if (extension.empty() || extension_list.empty()) return false;
  for (size_t i = 0; i < extension.length(); i++) {
    extension[i] = toupper(extension[i]);
  }
  for (size_t i = 0; i < extension_list.size(); i++) {
    if (extension == extension_list[i]) return true;
  }
  return false;
}

void CheckSlash(wstring& str) {
  if (str.length() > 0 && str[str.length() - 1] != '\\') str += L"\\";
}

wstring CheckSlash(const wstring& str) {
  if (str.length() > 0 && str[str.length() - 1] != '\\') {
    return str + L"\\";
  } else {
    return str;
  }
}

wstring GetFileExtension(const wstring str) {
  return str.substr(str.find_last_of(L".") + 1);
}

wstring GetFileName(const wstring str) {
  return str.substr(str.find_last_of(L"/\\") + 1);
}

wstring GetPathOnly(const wstring str) {
  return str.substr(0, str.find_last_of(L"/\\") + 1);
}

bool ValidateFileExtension(const wstring& extension, unsigned int max_length) {
  if (max_length > 0 && extension.length() > max_length) return false;
  if (!IsAlphanumeric(extension)) return false;
  return true;
}

// =============================================================================

/* Other */

wstring PushString(const wstring& str1, const wstring& str2) {
  if (str2.empty()) {
    return L"";
  } else {
    return str1 + str2;
  }
}

void ReadStringTable(UINT uID, wstring& str) {
  wchar_t buffer[2048];
  LoadString(g_hInstance, uID, buffer, 2048);
  str.append(buffer);
}

// =============================================================================

// Returns the most common non-alphanumeric character in a string that is included in the table.
// Characters in the table are listed by their precedence.

LPCWSTR COMMON_CHAR_TABLE = L",_ -+;.&|~";

int GetCommonCharIndex(wchar_t c) {
  for (int i = 0; i < 10; i++)
    if (COMMON_CHAR_TABLE[i] == c)
      return i;
  return -1;
}

wchar_t GetMostCommonCharacter(const wstring& str) {
  struct type_map { wchar_t ch; int count; };
  vector<type_map> char_map;
  int char_index = -1;

  for (size_t i = 0; i < str.length(); i++) {
    if (!IsAlphanumeric(str[i])) {
      char_index = GetCommonCharIndex(str[i]);
      if (char_index == -1) continue;
      for (size_t j = 0; j < char_map.size(); j++) {
        if (char_map[j].ch == str[i]) {
          char_map[j].count++;
          char_index = -1;
          break;
        }
      }
      if (char_index > -1) {
        size_t size = char_map.size();
        char_map.resize(size + 1);
        char_map[size].ch = str[i];
        char_map[size].count = 1;
      }
    }
  }
  
  char_index = 0;
  for (size_t i = 0; i < char_map.size(); i++) {
    if (char_map[i].count * 2 >= char_map[char_index].count &&
      GetCommonCharIndex(char_map[i].ch) < GetCommonCharIndex(char_map[char_index].ch)) {
        char_index = i;
    }
  }

  return char_map.empty() ? '\0' : char_map[char_index].ch;
}