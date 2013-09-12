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

#include "std.h"
#include "string.h"
#include <functional>
#include <iomanip>
#include <locale>
#include <sstream>

// =============================================================================

/* Erasing */

void Erase(wstring& str1, const wstring& str2, bool case_insensitive) {
  if (str2.empty() || str1.length() < str2.length()) return;
  auto it = str1.begin();
  do {
    if (case_insensitive) {
      it = std::search(it, str1.end(), str2.begin(), str2.end(), &IsCharsEqual);
    } else {
      it = std::search(it, str1.end(), str2.begin(), str2.end());
    }
    if (it != str1.end()) str1.erase(it, it + str2.length());
  } while (it != str1.end());
}

void EraseChars(wstring& str, const wchar_t chars[]) {
  size_t pos = 0;
  do {
    pos = str.find_first_of(chars, pos);
    if (pos != wstring::npos) str.erase(pos, 1);
  } while (pos != wstring::npos);
}

void ErasePunctuation(wstring& str, bool keep_trailing) {
  auto rlast = str.rbegin();
  if (keep_trailing) {
    rlast = std::find_if(str.rbegin(), str.rend(), 
    [](wchar_t c) -> bool {
      return !(c == L'!' || // "Hayate no Gotoku!", "K-ON!"...
               c == L'+' || // "Needless+"
               c == L'\''); // "Gintama'"
    });
  }

  auto it = std::remove_if(str.begin(), rlast.base(), 
    [](int c) -> bool {
      // Control codes, white-space and punctuation characters
      if (c <= 255 && !isalnum(c)) return true;
      // Unicode stars, hearts, notes, etc. (0x2000-0x2767)
      if (c > 8192 && c < 10087) return true;
      // Valid character
      return false;
    });
  
  if (keep_trailing) std::copy(rlast.base(), str.end(), it);
  str.resize(str.size() - (rlast.base() - it));
}

void EraseLeft(wstring& str1, const wstring str2, bool case_insensitive) {
  if (str1.length() < str2.length()) return;
  if (case_insensitive) {
    if (!std::equal(str2.begin(), str2.end(), str1.begin(), &IsCharsEqual)) return;
  } else {
    if (!std::equal(str2.begin(), str2.end(), str1.begin())) return;
  }
  str1.erase(str1.begin(), str1.begin() + str2.length());
}

void EraseRight(wstring& str1, const wstring str2, bool case_insensitive) {
  if (str1.length() < str2.length()) return;
  if (case_insensitive) {
    if (!std::equal(str2.begin(), str2.end(), str1.end() - str2.length(), &IsCharsEqual)) return;
  } else {
    if (!std::equal(str2.begin(), str2.end(), str1.end() - str2.length())) return;
  }
  str1.resize(str1.length() - str2.length());
}

void RemoveEmptyStrings(vector<wstring>& input) {
  if (input.empty()) return;
  input.erase(std::remove_if(input.begin(), input.end(),
    [](const wstring& s) -> bool { return s.empty(); }), 
    input.end());
}

// =============================================================================

/* Searching and comparison */

wstring CharLeft(const wstring& str, int length) {
  return str.substr(0, length);
}

wstring CharRight(const wstring& str, int length) {
  if (length > static_cast<int>(str.length())) {
    return str.substr(0, str.length());
  } else {
    return str.substr(str.length() - length, length);
  }
}

int CompareStrings(const wstring& str1, const wstring& str2, bool case_insensitive, size_t max_count) {
  if (case_insensitive) {
    return _wcsnicmp(str1.c_str(), str2.c_str(), max_count);
  } else {
    return wcsncmp(str1.c_str(), str2.c_str(), max_count);
  }
}

int InStr(const wstring& str1, const wstring str2, int pos, bool case_insensitive) {
  if (str1.empty()) return -1; if (str2.empty()) return 0;
  if (str1.length() < str2.length()) return -1;
  if (case_insensitive) {
    auto i = std::search(str1.begin() + pos, str1.end(), str2.begin(), str2.end(), &IsCharsEqual);
    return (i == str1.end()) ? -1 : i - str1.begin();
  } else {
    size_t i = str1.find(str2, pos);
    return (i != wstring::npos) ? i : -1;
  }
}

wstring InStr(const wstring& str1, const wstring& str2_left, const wstring& str2_right) {
  wstring output;
  int index_begin = InStr(str1, str2_left);
  if (index_begin > -1) {
    index_begin += str2_left.length();
    int index_end = InStr(str1, str2_right, index_begin);
    if (index_end > -1) {
      output = str1.substr(index_begin, index_end - index_begin);
    }
  }
  return output;
}

int InStrRev(const wstring& str1, const wstring str2, int pos) {
  size_t i = str1.rfind(str2, pos);
  return (i != wstring::npos) ? i : -1;
}

int InStrChars(const wstring& str1, const wstring str2, int pos) {
  size_t i = str1.find_first_of(str2, pos);
  return (i != wstring::npos) ? i : -1;
}

int InStrCharsRev(const wstring& str1, const wstring str2, int pos) {
  size_t i = str1.find_last_of(str2, pos);
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

inline bool IsCharsEqual(const wchar_t c1, const wchar_t c2) {
  return tolower(c1) == tolower(c2);
}

bool IsEqual(const wstring& str1, const wstring& str2) {
  if (str1.length() != str2.length()) return false;
  return std::equal(str1.begin(), str1.end(), str2.begin(), &IsCharsEqual);
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

bool StartsWith(const wstring& str1, const wstring& str2) {
  return str1.compare(0, str2.length(), str2) == 0;
}

bool EndsWith(const wstring& str1, const wstring& str2) {
  if (str2.length() > str1.length()) return false;
  return str1.compare(str1.length() - str2.length(), str2.length(), str2) == 0;
}

size_t LongestCommonSubsequenceLength(const wstring& str1, const wstring& str2) {
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
        table[i + 1][j + 1] = max(table[i + 1][j], table[i][j + 1]);
      }
    }
  }

  return table.back().back();
}

size_t LongestCommonSubstringLength(const wstring& str1, const wstring& str2) {
  if (str1.empty() || str2.empty())
    return 0;

  const size_t len1 = str1.length();
  const size_t len2 = str2.length();

  vector<vector<size_t>> table(len1);
  for (auto it = table.begin(); it != table.end(); ++it)
    it->resize(len2);

  size_t longest_length = 0;

  for (size_t i = 0; i < len1; i++) {
    for (size_t j = 0; j < len2; j++) {
      if (str1[i] == str2[j]) {
        if (i == 0 || j == 0) {
          table[i][j] = 1;
        } else {
          table[i][j] = table[i - 1][j - 1] + 1;
        }
        if (table[i][j] > longest_length) {
          longest_length = table[i][j];
        }
      } else {
        table[i][j] = 0;
      }
    }
  }

  return longest_length;
}

size_t LevenshteinDistance(const wstring& str1, const wstring& str2) {
  const size_t len1 = str1.size();
  const size_t len2 = str2.size();

  vector<size_t> prev_col(len2 + 1);
  for (size_t i = 0; i < prev_col.size(); i++)
    prev_col[i] = i;

  vector<size_t> col(len2 + 1);

  for (size_t i = 0; i < len1; i++) {
    col[0] = i + 1;

    for (size_t j = 0; j < len2; j++)
      col[j + 1] = min(min(1 + col[j], 1 + prev_col[1 + j]),
                       prev_col[j] + (str1[i] == str2[j] ? 0 : 1));

    col.swap(prev_col);
  }

  return prev_col[len2];
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

void ReplaceChar(wstring& str, const wchar_t c, const wchar_t replace_with) {
  if (c == replace_with) return;
  size_t pos = 0;
  do {
    pos = str.find_first_of(c, pos);
    if (pos != wstring::npos) str.at(pos) = replace_with;
  } while (pos != wstring::npos);
}

void ReplaceChars(wstring& str, const wchar_t chars[], const wstring replace_with) {
  if (chars == replace_with) return;
  size_t pos = 0;
  do {
    pos = str.find_first_of(chars, pos);
    if (pos != wstring::npos) str.replace(pos, 1, replace_with);
  } while (pos != wstring::npos);
}

// =============================================================================

/* Split, tokenize */

wstring Join(const vector<wstring>& join_vector, const wstring& separator) {
  wstring str;
  for (auto it = join_vector.begin(); it != join_vector.end(); ++it) {
    if (it != join_vector.begin()) str += separator;
    str += *it;
  }
  return str;
}

void Split(const wstring& str, const wstring& separator, std::vector<wstring>& split_vector) {
  if (separator.empty()) {
    split_vector.push_back(str);
    return;
  }
  size_t index_begin = 0, index_end;
  do {
    index_end = str.find(separator, index_begin);
    if (index_end == wstring::npos) index_end = str.length();
    split_vector.push_back(str.substr(index_begin, index_end - index_begin));
    index_begin = index_end + separator.length();
  } while (index_begin <= str.length());
}

wstring SubStr(const wstring& str, const wstring& sub_begin, const wstring& sub_end) {
  size_t index_begin = str.find(sub_begin, 0);
  if (index_begin == wstring::npos) return L"";
  size_t index_end = str.find(sub_end, index_begin);
  if (index_end == wstring::npos) index_end = str.length();
  return str.substr(index_begin + 1, index_end - index_begin - 1);
}

size_t Tokenize(const wstring& str, const wstring& delimiters, vector<wstring>& tokens) {
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

// =============================================================================

/* ANSI <-> Unicode conversion */

const char* ToANSI(const wstring& str, UINT code_page) {
  if (str.empty()) return "";
  int length = WideCharToMultiByte(code_page, 0, str.c_str(), -1, NULL, 0, NULL, NULL);
  if (length > 0) {
    char* output = new char[length + 1];
    WideCharToMultiByte(code_page, 0, str.c_str(), -1, output, length, NULL, NULL);
    return output;
  } else {
    return "";
  }
}

wstring ToUTF8(const string& str, UINT code_page) {
  if (str.empty()) return L"";
  int length = MultiByteToWideChar(code_page, 0, str.c_str(), -1, NULL, 0);
  if (length > 0) {
    vector<wchar_t> output(length);
    MultiByteToWideChar(code_page, 0, str.c_str(), -1, &output[0], length);
    return &output[0];
  } else {
    return L"";
  }
}

// =============================================================================

/* Case conversion */

// System-default ANSI code page
std::locale current_locale("");

void ToLower(wstring& str, bool use_locale) {
  if (use_locale) {
    std::transform(str.begin(), str.end(), str.begin(), 
      std::bind2nd(std::ptr_fun(&std::tolower<wchar_t>), current_locale));
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
      std::bind2nd(std::ptr_fun(&std::toupper<wchar_t>), current_locale));
  } else {
    std::transform(str.begin(), str.end(), str.begin(), towupper);
  }
}

wstring ToUpper_Copy(wstring str, bool use_locale) {
  ToUpper(str, use_locale);
  return str;
}

// =============================================================================

/* Type conversion */

int ToInt(const wstring& str) {
  return _wtoi(str.c_str());
}

wstring ToWstr(const INT& value) {
  wchar_t buffer[65];
  _ltow_s(value, buffer, 65, 10);
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

wstring ToWstr(const double& value, int count) {
  std::wostringstream out;
  out << std::fixed << std::setprecision(count) << value;
  return out.str();
}

// =============================================================================

/* Encoding & Decoding */

wstring EncodeUrl(const wstring& str, bool encode_unreserved) {
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

void DecodeHtmlEntities(wstring& str) {
  static std::map<wstring, wchar_t> html_entities;

  // Build entity map
  // Source: http://www.w3.org/TR/html4/sgml/entities.html
  if (html_entities.empty()) {
    // ISO 8859-1 characters
    html_entities[L"nbsp"] =     L'\u00A0';
    html_entities[L"iexcl"] =    L'\u00A1';
    html_entities[L"cent"] =     L'\u00A2';
    html_entities[L"pound"] =    L'\u00A3';
    html_entities[L"curren"] =   L'\u00A4';
    html_entities[L"yen"] =      L'\u00A5';
    html_entities[L"brvbar"] =   L'\u00A6';
    html_entities[L"sect"] =     L'\u00A7';
    html_entities[L"uml"] =      L'\u00A8';
    html_entities[L"copy"] =     L'\u00A9';
    html_entities[L"ordf"] =     L'\u00AA';
    html_entities[L"laquo"] =    L'\u00AB';
    html_entities[L"not"] =      L'\u00AC';
    html_entities[L"shy"] =      L'\u00AD';
    html_entities[L"reg"] =      L'\u00AE';
    html_entities[L"macr"] =     L'\u00AF';
    html_entities[L"deg"] =      L'\u00B0';
    html_entities[L"plusmn"] =   L'\u00B1';
    html_entities[L"sup2"] =     L'\u00B2';
    html_entities[L"sup3"] =     L'\u00B3';
    html_entities[L"acute"] =    L'\u00B4';
    html_entities[L"micro"] =    L'\u00B5';
    html_entities[L"para"] =     L'\u00B6';
    html_entities[L"middot"] =   L'\u00B7';
    html_entities[L"cedil"] =    L'\u00B8';
    html_entities[L"sup1"] =     L'\u00B9';
    html_entities[L"ordm"] =     L'\u00BA';
    html_entities[L"raquo"] =    L'\u00BB';
    html_entities[L"frac14"] =   L'\u00BC';
    html_entities[L"frac12"] =   L'\u00BD';
    html_entities[L"frac34"] =   L'\u00BE';
    html_entities[L"iquest"] =   L'\u00BF';
    html_entities[L"Agrave"] =   L'\u00C0';
    html_entities[L"Aacute"] =   L'\u00C1';
    html_entities[L"Acirc"] =    L'\u00C2';
    html_entities[L"Atilde"] =   L'\u00C3';
    html_entities[L"Auml"] =     L'\u00C4';
    html_entities[L"Aring"] =    L'\u00C5';
    html_entities[L"AElig"] =    L'\u00C6';
    html_entities[L"Ccedil"] =   L'\u00C7';
    html_entities[L"Egrave"] =   L'\u00C8';
    html_entities[L"Eacute"] =   L'\u00C9';
    html_entities[L"Ecirc"] =    L'\u00CA';
    html_entities[L"Euml"] =     L'\u00CB';
    html_entities[L"Igrave"] =   L'\u00CC';
    html_entities[L"Iacute"] =   L'\u00CD';
    html_entities[L"Icirc"] =    L'\u00CE';
    html_entities[L"Iuml"] =     L'\u00CF';
    html_entities[L"ETH"] =      L'\u00D0';
    html_entities[L"Ntilde"] =   L'\u00D1';
    html_entities[L"Ograve"] =   L'\u00D2';
    html_entities[L"Oacute"] =   L'\u00D3';
    html_entities[L"Ocirc"] =    L'\u00D4';
    html_entities[L"Otilde"] =   L'\u00D5';
    html_entities[L"Ouml"] =     L'\u00D6';
    html_entities[L"times"] =    L'\u00D7';
    html_entities[L"Oslash"] =   L'\u00D8';
    html_entities[L"Ugrave"] =   L'\u00D9';
    html_entities[L"Uacute"] =   L'\u00DA';
    html_entities[L"Ucirc"] =    L'\u00DB';
    html_entities[L"Uuml"] =     L'\u00DC';
    html_entities[L"Yacute"] =   L'\u00DD';
    html_entities[L"THORN"] =    L'\u00DE';
    html_entities[L"szlig"] =    L'\u00DF';
    html_entities[L"agrave"] =   L'\u00E0';
    html_entities[L"aacute"] =   L'\u00E1';
    html_entities[L"acirc"] =    L'\u00E2';
    html_entities[L"atilde"] =   L'\u00E3';
    html_entities[L"auml"] =     L'\u00E4';
    html_entities[L"aring"] =    L'\u00E5';
    html_entities[L"aelig"] =    L'\u00E6';
    html_entities[L"ccedil"] =   L'\u00E7';
    html_entities[L"egrave"] =   L'\u00E8';
    html_entities[L"eacute"] =   L'\u00E9';
    html_entities[L"ecirc"] =    L'\u00EA';
    html_entities[L"euml"] =     L'\u00EB';
    html_entities[L"igrave"] =   L'\u00EC';
    html_entities[L"iacute"] =   L'\u00ED';
    html_entities[L"icirc"] =    L'\u00EE';
    html_entities[L"iuml"] =     L'\u00EF';
    html_entities[L"eth"] =      L'\u00F0';
    html_entities[L"ntilde"] =   L'\u00F1';
    html_entities[L"ograve"] =   L'\u00F2';
    html_entities[L"oacute"] =   L'\u00F3';
    html_entities[L"ocirc"] =    L'\u00F4';
    html_entities[L"otilde"] =   L'\u00F5';
    html_entities[L"ouml"] =     L'\u00F6';
    html_entities[L"divide"] =   L'\u00F7';
    html_entities[L"oslash"] =   L'\u00F8';
    html_entities[L"ugrave"] =   L'\u00F9';
    html_entities[L"uacute"] =   L'\u00FA';
    html_entities[L"ucirc"] =    L'\u00FB';
    html_entities[L"uuml"] =     L'\u00FC';
    html_entities[L"yacute"] =   L'\u00FD';
    html_entities[L"thorn"] =    L'\u00FE';
    html_entities[L"yuml"] =     L'\u00FF';
    // Symbols, mathematical symbols, and Greek letters
    // Latin Extended-B
    html_entities[L"fnof"] =     L'\u0192';
    // Greek
    html_entities[L"Alpha"] =    L'\u0391';
    html_entities[L"Beta"] =     L'\u0392';
    html_entities[L"Gamma"] =    L'\u0393';
    html_entities[L"Delta"] =    L'\u0394';
    html_entities[L"Epsilon"] =  L'\u0395';
    html_entities[L"Zeta"] =     L'\u0396';
    html_entities[L"Eta"] =      L'\u0397';
    html_entities[L"Theta"] =    L'\u0398';
    html_entities[L"Iota"] =     L'\u0399';
    html_entities[L"Kappa"] =    L'\u039A';
    html_entities[L"Lambda"] =   L'\u039B';
    html_entities[L"Mu"] =       L'\u039C';
    html_entities[L"Nu"] =       L'\u039D';
    html_entities[L"Xi"] =       L'\u039E';
    html_entities[L"Omicron"] =  L'\u039F';
    html_entities[L"Pi"] =       L'\u03A0';
    html_entities[L"Rho"] =      L'\u03A1';
    html_entities[L"Sigma"] =    L'\u03A3';
    html_entities[L"Tau"] =      L'\u03A4';
    html_entities[L"Upsilon"] =  L'\u03A5';
    html_entities[L"Phi"] =      L'\u03A6';
    html_entities[L"Chi"] =      L'\u03A7';
    html_entities[L"Psi"] =      L'\u03A8';
    html_entities[L"Omega"] =    L'\u03A9';
    html_entities[L"alpha"] =    L'\u03B1';
    html_entities[L"beta"] =     L'\u03B2';
    html_entities[L"gamma"] =    L'\u03B3';
    html_entities[L"delta"] =    L'\u03B4';
    html_entities[L"epsilon"] =  L'\u03B5';
    html_entities[L"zeta"] =     L'\u03B6';
    html_entities[L"eta"] =      L'\u03B7';
    html_entities[L"theta"] =    L'\u03B8';
    html_entities[L"iota"] =     L'\u03B9';
    html_entities[L"kappa"] =    L'\u03BA';
    html_entities[L"lambda"] =   L'\u03BB';
    html_entities[L"mu"] =       L'\u03BC';
    html_entities[L"nu"] =       L'\u03BD';
    html_entities[L"xi"] =       L'\u03BE';
    html_entities[L"omicron"] =  L'\u03BF';
    html_entities[L"pi"] =       L'\u03C0';
    html_entities[L"rho"] =      L'\u03C1';
    html_entities[L"sigmaf"] =   L'\u03C2';
    html_entities[L"sigma"] =    L'\u03C3';
    html_entities[L"tau"] =      L'\u03C4';
    html_entities[L"upsilon"] =  L'\u03C5';
    html_entities[L"phi"] =      L'\u03C6';
    html_entities[L"chi"] =      L'\u03C7';
    html_entities[L"psi"] =      L'\u03C8';
    html_entities[L"omega"] =    L'\u03C9';
    html_entities[L"thetasym"] = L'\u03D1';
    html_entities[L"upsih"] =    L'\u03D2';
    html_entities[L"piv"] =      L'\u03D6';
    // General Punctuation
    html_entities[L"bull"] =     L'\u2022';
    html_entities[L"hellip"] =   L'\u2026';
    html_entities[L"prime"] =    L'\u2032';
    html_entities[L"Prime"] =    L'\u2033';
    html_entities[L"oline"] =    L'\u203E';
    html_entities[L"frasl"] =    L'\u2044';
    // Letterlike Symbols
    html_entities[L"weierp"] =   L'\u2118';
    html_entities[L"image"] =    L'\u2111';
    html_entities[L"real"] =     L'\u211C';
    html_entities[L"trade"] =    L'\u2122';
    html_entities[L"alefsym"] =  L'\u2135';
    // Arrows
    html_entities[L"larr"] =     L'\u2190';
    html_entities[L"uarr"] =     L'\u2191';
    html_entities[L"rarr"] =     L'\u2192';
    html_entities[L"darr"] =     L'\u2193';
    html_entities[L"harr"] =     L'\u2194';
    html_entities[L"crarr"] =    L'\u21B5';
    html_entities[L"lArr"] =     L'\u21D0';
    html_entities[L"uArr"] =     L'\u21D1';
    html_entities[L"rArr"] =     L'\u21D2';
    html_entities[L"dArr"] =     L'\u21D3';
    html_entities[L"hArr"] =     L'\u21D4';
    // Mathematical Operators
    html_entities[L"forall"] =   L'\u2200';
    html_entities[L"part"] =     L'\u2202';
    html_entities[L"exist"] =    L'\u2203';
    html_entities[L"empty"] =    L'\u2205';
    html_entities[L"nabla"] =    L'\u2207';
    html_entities[L"isin"] =     L'\u2208';
    html_entities[L"notin"] =    L'\u2209';
    html_entities[L"ni"] =       L'\u220B';
    html_entities[L"prod"] =     L'\u220F';
    html_entities[L"sum"] =      L'\u2211';
    html_entities[L"minus"] =    L'\u2212';
    html_entities[L"lowast"] =   L'\u2217';
    html_entities[L"radic"] =    L'\u221A';
    html_entities[L"prop"] =     L'\u221D';
    html_entities[L"infin"] =    L'\u221E';
    html_entities[L"ang"] =      L'\u2220';
    html_entities[L"and"] =      L'\u2227';
    html_entities[L"or"] =       L'\u2228';
    html_entities[L"cap"] =      L'\u2229';
    html_entities[L"cup"] =      L'\u222A';
    html_entities[L"int"] =      L'\u222B';
    html_entities[L"there4"] =   L'\u2234';
    html_entities[L"sim"] =      L'\u223C';
    html_entities[L"cong"] =     L'\u2245';
    html_entities[L"asymp"] =    L'\u2248';
    html_entities[L"ne"] =       L'\u2260';
    html_entities[L"equiv"] =    L'\u2261';
    html_entities[L"le"] =       L'\u2264';
    html_entities[L"ge"] =       L'\u2265';
    html_entities[L"sub"] =      L'\u2282';
    html_entities[L"sup"] =      L'\u2283';
    html_entities[L"nsub"] =     L'\u2284';
    html_entities[L"sube"] =     L'\u2286';
    html_entities[L"supe"] =     L'\u2287';
    html_entities[L"oplus"] =    L'\u2295';
    html_entities[L"otimes"] =   L'\u2297';
    html_entities[L"perp"] =     L'\u22A5';
    html_entities[L"sdot"] =     L'\u22C5';
    // Miscellaneous Technical
    html_entities[L"lceil"] =    L'\u2308';
    html_entities[L"rceil"] =    L'\u2309';
    html_entities[L"lfloor"] =   L'\u230A';
    html_entities[L"rfloor"] =   L'\u230B';
    html_entities[L"lang"] =     L'\u2329';
    html_entities[L"rang"] =     L'\u232A';
    // Geometric Shapes
    html_entities[L"loz"] =      L'\u25CA';
    // Miscellaneous Symbols
    html_entities[L"spades"] =   L'\u2660';
    html_entities[L"clubs"] =    L'\u2663';
    html_entities[L"hearts"] =   L'\u2665';
    html_entities[L"diams"] =    L'\u2666';
    // Markup-significant and internationalization characters
    // C0 Controls and Basic Latin
    html_entities[L"quot"] =     L'\"';
    html_entities[L"amp"] =      L'&';
    html_entities[L"lt"] =       L'<';
    html_entities[L"gt"] =       L'>';
    // Latin Extended-A
    html_entities[L"OElig"] =    L'\u0152';
    html_entities[L"oelig"] =    L'\u0153';
    html_entities[L"Scaron"] =   L'\u0160';
    html_entities[L"scaron"] =   L'\u0161';
    html_entities[L"Yuml"] =     L'\u0178';
    // Spacing Modifier Letters
    html_entities[L"circ"] =     L'\u02C6';
    html_entities[L"tilde"] =    L'\u02DC';
    // General Punctuation
    html_entities[L"ensp"] =     L'\u2002';
    html_entities[L"emsp"] =     L'\u2003';
    html_entities[L"thinsp"] =   L'\u2009';
    html_entities[L"zwnj"] =     L'\u200C';
    html_entities[L"zwj"] =      L'\u200D';
    html_entities[L"lrm"] =      L'\u200E';
    html_entities[L"rlm"] =      L'\u200F';
    html_entities[L"ndash"] =    L'\u2013';
    html_entities[L"mdash"] =    L'\u2014';
    html_entities[L"lsquo"] =    L'\u2018';
    html_entities[L"rsquo"] =    L'\u2019';
    html_entities[L"sbquo"] =    L'\u201A';
    html_entities[L"ldquo"] =    L'\u201C';
    html_entities[L"rdquo"] =    L'\u201D';
    html_entities[L"bdquo"] =    L'\u201E';
    html_entities[L"dagger"] =   L'\u2020';
    html_entities[L"Dagger"] =   L'\u2021';
    html_entities[L"permil"] =   L'\u2030';
    html_entities[L"lsaquo"] =   L'\u2039';
    html_entities[L"rsaquo"] =   L'\u203A';
    html_entities[L"euro"] =     L'\u20AC';
  }

  if (InStr(str, L"&") == -1) return;
  
  size_t pos = 0, reference_pos = 0;
  unsigned int character_value = -1;

  for (size_t i = 0; i < str.size(); i++) {
    if (str.at(i) == L'&') {
      reference_pos = i;
      character_value = -1;
      if (++i == str.size()) return;
      
      // Numeric character references
      if (str.at(i) == L'#') {
        if (++i == str.size()) return;
        // Hexadecimal (&#xhhhh;)
        if (str.at(i) == L'x') {
          if (++i == str.size()) return;
          pos = i;
          while (i < str.size() && IsHex(str.at(i))) i++;
          if (i > pos && i < str.size() && str.at(i) == L';') {
            character_value = wcstoul(str.substr(pos, i - pos).c_str(), nullptr, 16);
          }
        // Decimal (&#nnnn;)
        } else {
          pos = i;
          while (i < str.size() && IsNumeric(str.at(i))) i++;
          if (i > pos && i < str.size() && str.at(i) == L';') {
            character_value = ToInt(str.substr(pos, i - pos));
          }
        }

      // Character entity references
      } else {
        pos = i;
        while (i < str.size() && IsAlphanumeric(str.at(i))) i++;
        if (i > pos && i < str.size() && str.at(i) == L';') {
          size_t length = i - pos;
          if (length > 1 && length < 10) {
            wstring entity_name = str.substr(pos, length);
            if (html_entities.find(entity_name) != html_entities.end()) {
              character_value = html_entities[entity_name];
            }
          }
        }
      }

      if (character_value <= 0xFFFD) {
        str.replace(reference_pos, i - reference_pos + 1, 
                    wstring(1, static_cast<wchar_t>(character_value)));
        i = reference_pos - 1;
      } else {
        i = reference_pos;
      }
    }
  }
}

void StripHtmlTags(wstring& str) {
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

void LimitText(wstring& str, unsigned int limit, const wstring& tail) {
  if (str.length() > limit) {
    str.resize(limit);
    if (!tail.empty() && str.length() > tail.length()) {
      str.replace(str.length() - tail.length(), tail.length(), tail);
    }
  }
}

void Trim(wstring& str, const wchar_t trim_chars[], bool trim_left, bool trim_right) {
  if (str.empty()) return;
  const size_t index_begin = trim_left ? str.find_first_not_of(trim_chars) : 0;
  const size_t index_end = trim_right ? str.find_last_not_of(trim_chars) : str.length() - 1;
  if (index_begin == wstring::npos || index_end == wstring::npos) {
    str.clear();
    return;
  }
  if (trim_right) str.erase(index_end + 1, str.length() - index_end + 1);
  if (trim_left) str.erase(0, index_begin);
}

void TrimLeft(wstring& str, const wchar_t trim_chars[]) {
  Trim(str, trim_chars, true, false);
}

void TrimRight(wstring& str, const wchar_t trim_chars[]) {
  Trim(str, trim_chars, false, true);
}

// =============================================================================

/* File and folder related */

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

wstring GetFileExtension(const wstring& str) {
  return str.substr(str.find_last_of(L".") + 1);
}

wstring GetFileWithoutExtension(wstring str) {
  size_t pos = str.find_last_of(L".");
  if (pos != wstring::npos) str.resize(pos);
  return str;
}

wstring GetFileName(const wstring& str) {
  return str.substr(str.find_last_of(L"/\\") + 1);
}

wstring GetPathOnly(const wstring& str) {
  return str.substr(0, str.find_last_of(L"/\\") + 1);
}

bool ValidateFileExtension(const wstring& extension, unsigned int max_length) {
  if (max_length > 0 && extension.length() > max_length) return false;
  if (!IsAlphanumeric(extension)) return false;
  return true;
}

// =============================================================================

/* Other */

void AppendString(wstring& str0, const wstring& str1, const wstring& str2) {
  if (str1.empty()) return;
  if (!str0.empty()) str0.append(str2);
  str0.append(str1);
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

void ReadStringFromResource(LPCWSTR name, LPCWSTR type, wstring& output) {
  HRSRC hResInfo = FindResource(nullptr, name,  type);
  HGLOBAL hResHandle = LoadResource(nullptr, hResInfo);
  DWORD dwSize = SizeofResource(nullptr, hResInfo);
  
  const char* lpData = static_cast<char*>(LockResource(hResHandle));
  string temp(lpData, dwSize);
  output = ToUTF8(temp);

  FreeResource(hResInfo);
}

void ReadStringTable(UINT uID, wstring& str) {
  wchar_t buffer[2048];
  LoadString(g_hInstance, uID, buffer, 2048);
  str.append(buffer);
}

// =============================================================================

// Returns the most common non-alphanumeric character in a string that is included in the table.
// Characters in the table are listed by their precedence.

const wchar_t* COMMON_CHAR_TABLE = L",_ -+;.&|~";

int GetCommonCharIndex(wchar_t c) {
  for (int i = 0; i < 10; i++)
    if (COMMON_CHAR_TABLE[i] == c)
      return i;
  return -1;
}

wchar_t GetMostCommonCharacter(const wstring& str) {
  vector<std::pair<wchar_t, int>> char_map;
  int char_index = -1;

  for (size_t i = 0; i < str.length(); i++) {
    if (!IsAlphanumeric(str[i])) {
      char_index = GetCommonCharIndex(str[i]);
      if (char_index == -1) continue;
      for (auto it = char_map.begin(); it != char_map.end(); ++it) {
        if (it->first == str[i]) {
          it->second++;
          char_index = -1;
          break;
        }
      }
      if (char_index > -1) {
        char_map.push_back(std::make_pair(str[i], 1));
      }
    }
  }
  
  char_index = 0;
  for (auto it = char_map.begin(); it != char_map.end(); ++it) {
    if (it->second * 1.8f >= char_map.at(char_index).second &&
        GetCommonCharIndex(it->first) < GetCommonCharIndex(char_map.at(char_index).first)) {
      char_index = it - char_map.begin();
    }
  }

  return char_map.empty() ? '\0' : char_map.at(char_index).first;
}