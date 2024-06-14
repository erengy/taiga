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

#include <utf8proc/utf8proc.h>

#include "track/recognition.h"

namespace track::recognition {

void Engine::Normalize(std::wstring& title, int type,
                       bool normalized_before) const {
  bool modified_tail = false;

  if (!normalized_before) {
    const auto unmodified_title = title;

    ConvertRomanNumbers(title);
    Transliterate(title);
    NormalizeUnicode(title);  // Title is lower case after this point, due to UTF8PROC_CASEFOLD
    ConvertOrdinalNumbers(title);
    ConvertSeasonNumbers(title);
    EraseUnnecessary(title);
    Trim(title);

    if (title.size() != unmodified_title.size() &&
        title.back() != unmodified_title.back()) {
      modified_tail = true;
    }
  }

  switch (type) {
    case kNormalizeMinimal:
      break;
    case kNormalizeForTrigrams:
    case kNormalizeForLookup:
    case kNormalizeFull:
      ErasePunctuation(title, type, modified_tail);
      break;
  }

  if (type < kNormalizeFull)
    while (ReplaceString(title, 0, L"  ", L" ", false, true));
}

/////////////////////////////////////////////////////////////////////////////////

void Engine::ConvertOrdinalNumbers(std::wstring& str) const {
  static const std::vector<std::pair<std::wstring, std::wstring>> ordinals{
    {L"1st", L"first"}, {L"2nd", L"second"}, {L"3rd", L"third"},
    {L"4th", L"fourth"}, {L"5th", L"fifth"}, {L"6th", L"sixth"},
    {L"7th", L"seventh"}, {L"8th", L"eighth"}, {L"9th", L"ninth"},
  };

  for (const auto& ordinal : ordinals)
    ReplaceString(str, 0, ordinal.second, ordinal.first, true, true);
}

void Engine::ConvertRomanNumbers(std::wstring& str) const {
  // We skip 1 and 10 to avoid matching "I" and "X", as they're unlikely to be
  // used as Roman numerals. Any number above "XIII" is rarely used in anime
  // titles, which is why we don't need an actual Roman-to-Arabic number
  // conversion algorithm.
  static const std::vector<std::pair<std::wstring, std::wstring>> numerals{
    {L"2", L"II"}, {L"3", L"III"}, {L"4", L"IV"}, {L"5", L"V"},
    {L"6", L"VI"}, {L"7", L"VII"}, {L"8", L"VIII"}, {L"9", L"IX"},
    {L"11", L"XI"}, {L"12", L"XII"}, {L"13", L"XIII"},
  };

  for (const auto& numeral : numerals)
    ReplaceString(str, 0, numeral.second, numeral.first, true, true);
}

void Engine::ConvertSeasonNumbers(std::wstring& str) const {
  // This works considerably faster than regular expressions.
  using season_t = std::vector<std::wstring>;
  static const std::vector<std::pair<std::wstring, season_t>> values{
    {L"1", {L"1st season", L"season 1", L"series 1", L"s1"}},
    {L"2", {L"2nd season", L"season 2", L"series 2", L"s2"}},
    {L"3", {L"3rd season", L"season 3", L"series 3", L"s3"}},
    {L"4", {L"4th season", L"season 4", L"series 4", L"s4"}},
    {L"5", {L"5th season", L"season 5", L"series 5", L"s5"}},
    {L"6", {L"6th season", L"season 6", L"series 6", L"s6"}},
  };

  for (const auto& value : values)
    for (const auto& season : value.second)
      ReplaceString(str, 0, season, value.first, true, true);
}

void Engine::Transliterate(std::wstring& str) const {
  for (size_t i = 0; i < str.size(); ++i) {
    auto& c = str[i];
    switch (c) {
      // Character equivalencies that are not included in UTF8PROC_LUMP
      case L'@': c = L'a'; break;  // e.g. "iDOLM@STER" (doesn't make a difference for "GJ-bu@" or "Sasami-san@Ganbaranai")
      case L'\u00D7': c = L'x'; break;  // multiplication sign (e.g. "Tasogare Otome x Amnesia")
      case L'\uA789': c = L':'; break;  // modifier letter colon (e.g. "Nisekoi:")
      // A few common always-equivalent romanizations
      case L'\u014C': str.replace(i, 1, L"ou"); break;  // latin capital letter o with macron
      case L'\u014D': str.replace(i, 1, L"ou"); break;  // latin small letter o with macron
      case L'\u016B': str.replace(i, 1, L"uu"); break;  // latin small letter u with macron
    }
  }

  // Romanizations (Hepburn to Wapuro)
  ReplaceString(str, 0, L"wa", L"ha", true, true);
  ReplaceString(str, 0, L"e", L"he", true, true);
  ReplaceString(str, 0, L"o", L"wo", true, true);
}

void Engine::NormalizeUnicode(std::wstring& str) const {
  constexpr int options =
      // NFKC normalization according to Unicode Standard Annex #15
      UTF8PROC_COMPAT | UTF8PROC_COMPOSE | UTF8PROC_STABLE |
      // Strip "default ignorable" characters, control characters, character
      // marks (accents, diaeresis)
      UTF8PROC_IGNORE | UTF8PROC_STRIPCC | UTF8PROC_STRIPMARK |
      // Map certain characters (e.g. hyphen and minus) for easier comparison
      UTF8PROC_LUMP |
      // Perform unicode case folding for case-insensitive comparison
      UTF8PROC_CASEFOLD;

  char* buffer = nullptr;
  std::string temp = WstrToStr(str);

  const auto length = utf8proc_map(
      reinterpret_cast<const utf8proc_uint8_t*>(temp.data()), temp.length(),
      reinterpret_cast<utf8proc_uint8_t**>(&buffer),
      static_cast<utf8proc_option_t>(options));

  if (length >= 0) {
    temp.assign(buffer, length);
    str = StrToWstr(temp);
  }

  if (buffer)
    free(buffer);
}

// TODO: Rename
void Engine::EraseUnnecessary(std::wstring& str) const {
  ReplaceString(str, 0, L"&", L"and", true, true);
  ReplaceString(str, 0, L"the animation", L"", true, true);
  ReplaceString(str, 0, L"the", L"", true, true);
  ReplaceString(str, 0, L"episode", L"", true, true);
  ReplaceString(str, 0, L"oad", L"ova", true, true);
  ReplaceString(str, 0, L"oav", L"ova", true, true);
  ReplaceString(str, 0, L"specials", L"sp", true, true);
  ReplaceString(str, 0, L"special", L"sp", true, true);
  ReplaceString(str, 0, L"(tv)", L"", true, true);
}

void Engine::ErasePunctuation(std::wstring& str, int type,
                              bool modified_tail) const {
  bool erase_tail = modified_tail || type == kNormalizeFull;
  bool erase_whitespace = type >= kNormalizeForLookup;
  bool is_tail = true;

  auto is_removable = [&](const wchar_t c) {
    // Control codes, white-space and punctuation characters
    if (c <= 0xFF && !IsAlphanumericChar(c)) {
      switch (c) {
        case L' ':
          return erase_whitespace;
        case L')':
        case L']':
          return !is_tail;
        default:
          return true;
      }
    // Unicode stars, hearts, notes, etc.
    } else if (c > 0x2000 && c < 0x2767) {
      return true;
    }
    // Valid character
    return false;
  };

  auto it_end = erase_tail ? str.end() :
      std::find_if_not(str.rbegin(), str.rend(), is_removable).base();
  is_tail = false;

  auto it = std::remove_if(str.begin(), it_end, is_removable);

  if (!erase_tail)
    it = std::copy(it_end, str.end(), it);

  if (it != str.end())
    str.erase(it, str.end());
}

}  // namespace track::recognition
