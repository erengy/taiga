/**
 * Taiga
 * Copyright (C) 2010-2026, Eren Okka
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

#include "recognition_normalize.hpp"

#include <utf8proc.h>

#include <QList>
#include <QString>
#include <cstdlib>
#include <ranges>

#include "base/string.hpp"

namespace track::recognition {

std::string normalize(const std::string& title) {
  auto str = QString::fromStdString(title);

  normalizeRomanNumbers(str);
  transliterate(str);

  normalizeUnicode(str);  // title is lower case after this point, due to UTF8PROC_CASEFOLD

  normalizeOrdinalNumbers(str);
  normalizeSeasonNumbers(str);

  replaceWholeWord(str, "&", "and");
  replaceWholeWord(str, "the animation", "");
  replaceWholeWord(str, "the", "");
  replaceWholeWord(str, "episode", "");
  replaceWholeWord(str, "oad", "ova");
  replaceWholeWord(str, "oav", "ova");
  replaceWholeWord(str, "specials", "sp");
  replaceWholeWord(str, "special", "sp");
  replaceWholeWord(str, "(tv)", "");

  str = str.simplified();
  erasePunctuation(str);

  return str.toStdString();
}

/////////////////////////////////////////////////////////////////////////////////

void erasePunctuation(QString& str) {
  static constexpr auto isRemovable = [&](const QChar& c) {
    // Control codes, white-space and punctuation characters
    if (c.unicode() <= 0xFF && !c.isLetterOrNumber()) return true;
    // Unicode stars, hearts, notes, etc.
    if (c.unicode() > 0x2000 && c.unicode() < 0x2767) return true;
    // Valid character
    return false;
  };

  str.removeIf(isRemovable);
}

void normalizeOrdinalNumbers(QString& str) {
  static const QList<QPair<const char*, const char*>> ordinals{
      // clang-format off
      {"first",   "1st"},
      {"second",  "2nd"},
      {"third",   "3rd"},
      {"fourth",  "4th"},
      {"fifth",   "5th"},
      {"sixth",   "6th"},
      {"seventh", "7th"},
      {"eighth",  "8th"},
      {"ninth",   "9th"},
      // clang-format on
  };

  for (auto [before, after] : ordinals) {
    replaceWholeWord(str, before, after);
  }
}

void normalizeRomanNumbers(QString& str) {
  // We skip 1 and 10 to avoid matching "I" and "X", as they're unlikely to be used as Roman
  // numerals. Any number above "XIII" is rarely used in anime titles, so we don't need an actual
  // Roman-to-Arabic number conversion algorithm.
  static const QList<QPair<const char*, const char*>> numerals{
      // clang-format off
      {"II",    "2"},
      {"III",   "3"},
      {"IV",    "4"},
      {"V",     "5"},
      {"VI",    "6"},
      {"VII",   "7"},
      {"VIII",  "8"},
      {"IX",    "9"},
      {"XI",   "11"},
      {"XII",  "12"},
      {"XIII", "13"},
      // clang-format on
  };

  for (auto [before, after] : numerals) {
    replaceWholeWord(str, before, after);
  }
}

void normalizeSeasonNumbers(QString& str) {
  // This works considerably faster than regular expressions.
  static const QList<QPair<const char*, QList<const char*>>> values{
      {"1", {"1st season", "season 1", "series 1", "s1"}},
      {"2", {"2nd season", "season 2", "series 2", "s2"}},
      {"3", {"3rd season", "season 3", "series 3", "s3"}},
      {"4", {"4th season", "season 4", "series 4", "s4"}},
      {"5", {"5th season", "season 5", "series 5", "s5"}},
      {"6", {"6th season", "season 6", "series 6", "s6"}},
  };

  for (const auto& [after, list] : values) {
    for (auto before : list) {
      replaceWholeWord(str, before, after);
    }
  }
}

void normalizeUnicode(QString& str) {
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
  const auto utf8str = str.toUtf8();

  const auto length = utf8proc_map(reinterpret_cast<const utf8proc_uint8_t*>(utf8str.data()),
                                   utf8str.length(), reinterpret_cast<utf8proc_uint8_t**>(&buffer),
                                   static_cast<utf8proc_option_t>(options));

  if (length >= 0) {
    str = QString::fromUtf8(buffer, length);
  }

  std::free(buffer);
}

void transliterate(QString& str) {
  for (qsizetype i = 0; i < str.size(); ++i) {
    auto& c = str[i];

    // clang-format off
    switch (c.unicode()) {
      // Character equivalencies that are not included in UTF8PROC_LUMP
      case u'@': c = u'a'; break;  // e.g. "iDOLM@STER" (doesn't make a difference for "GJ-bu@" or "Sasami-san@Ganbaranai")
      case u'×': c = u'x'; break;  // multiplication sign (e.g. "Tasogare Otome x Amnesia")
      case u'꞉': c = u':'; break;  // modifier letter colon (e.g. "Nisekoi:")

      // A few common always-equivalent romanizations
      // NOLINTNEXTLINE(bugprone-branch-clone)
      case u'Ō': str.replace(i, 1, "ou"); break;  // latin capital letter o with macron
      case u'ō': str.replace(i, 1, "ou"); break;  // latin small letter o with macron
      case u'ū': str.replace(i, 1, "uu"); break;  // latin small letter u with macron
    }
    // clang-format on
  }

  // Romanizations (Hepburn to Wapuro)
  replaceWholeWord(str, "wa", "ha");
  replaceWholeWord(str, "e", "he");
  replaceWholeWord(str, "o", "wo");
}

}  // namespace track::recognition
