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
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "base/html.h"

#include "base/string.h"

// Source: https://www.w3.org/TR/html4/sgml/entities.html
static const std::unordered_map<std::wstring_view, wchar_t> html_entities{
  //////////////////////////////////////////////////////////////////////////////
  // ISO 8859-1 characters

  {L"nbsp",     L'\u00A0'},
  {L"iexcl",    L'\u00A1'},
  {L"cent",     L'\u00A2'},
  {L"pound",    L'\u00A3'},
  {L"curren",   L'\u00A4'},
  {L"yen",      L'\u00A5'},
  {L"brvbar",   L'\u00A6'},
  {L"sect",     L'\u00A7'},
  {L"uml",      L'\u00A8'},
  {L"copy",     L'\u00A9'},
  {L"ordf",     L'\u00AA'},
  {L"laquo",    L'\u00AB'},
  {L"not",      L'\u00AC'},
  {L"shy",      L'\u00AD'},
  {L"reg",      L'\u00AE'},
  {L"macr",     L'\u00AF'},
  {L"deg",      L'\u00B0'},
  {L"plusmn",   L'\u00B1'},
  {L"sup2",     L'\u00B2'},
  {L"sup3",     L'\u00B3'},
  {L"acute",    L'\u00B4'},
  {L"micro",    L'\u00B5'},
  {L"para",     L'\u00B6'},
  {L"middot",   L'\u00B7'},
  {L"cedil",    L'\u00B8'},
  {L"sup1",     L'\u00B9'},
  {L"ordm",     L'\u00BA'},
  {L"raquo",    L'\u00BB'},
  {L"frac14",   L'\u00BC'},
  {L"frac12",   L'\u00BD'},
  {L"frac34",   L'\u00BE'},
  {L"iquest",   L'\u00BF'},
  {L"Agrave",   L'\u00C0'},
  {L"Aacute",   L'\u00C1'},
  {L"Acirc",    L'\u00C2'},
  {L"Atilde",   L'\u00C3'},
  {L"Auml",     L'\u00C4'},
  {L"Aring",    L'\u00C5'},
  {L"AElig",    L'\u00C6'},
  {L"Ccedil",   L'\u00C7'},
  {L"Egrave",   L'\u00C8'},
  {L"Eacute",   L'\u00C9'},
  {L"Ecirc",    L'\u00CA'},
  {L"Euml",     L'\u00CB'},
  {L"Igrave",   L'\u00CC'},
  {L"Iacute",   L'\u00CD'},
  {L"Icirc",    L'\u00CE'},
  {L"Iuml",     L'\u00CF'},
  {L"ETH",      L'\u00D0'},
  {L"Ntilde",   L'\u00D1'},
  {L"Ograve",   L'\u00D2'},
  {L"Oacute",   L'\u00D3'},
  {L"Ocirc",    L'\u00D4'},
  {L"Otilde",   L'\u00D5'},
  {L"Ouml",     L'\u00D6'},
  {L"times",    L'\u00D7'},
  {L"Oslash",   L'\u00D8'},
  {L"Ugrave",   L'\u00D9'},
  {L"Uacute",   L'\u00DA'},
  {L"Ucirc",    L'\u00DB'},
  {L"Uuml",     L'\u00DC'},
  {L"Yacute",   L'\u00DD'},
  {L"THORN",    L'\u00DE'},
  {L"szlig",    L'\u00DF'},
  {L"agrave",   L'\u00E0'},
  {L"aacute",   L'\u00E1'},
  {L"acirc",    L'\u00E2'},
  {L"atilde",   L'\u00E3'},
  {L"auml",     L'\u00E4'},
  {L"aring",    L'\u00E5'},
  {L"aelig",    L'\u00E6'},
  {L"ccedil",   L'\u00E7'},
  {L"egrave",   L'\u00E8'},
  {L"eacute",   L'\u00E9'},
  {L"ecirc",    L'\u00EA'},
  {L"euml",     L'\u00EB'},
  {L"igrave",   L'\u00EC'},
  {L"iacute",   L'\u00ED'},
  {L"icirc",    L'\u00EE'},
  {L"iuml",     L'\u00EF'},
  {L"eth",      L'\u00F0'},
  {L"ntilde",   L'\u00F1'},
  {L"ograve",   L'\u00F2'},
  {L"oacute",   L'\u00F3'},
  {L"ocirc",    L'\u00F4'},
  {L"otilde",   L'\u00F5'},
  {L"ouml",     L'\u00F6'},
  {L"divide",   L'\u00F7'},
  {L"oslash",   L'\u00F8'},
  {L"ugrave",   L'\u00F9'},
  {L"uacute",   L'\u00FA'},
  {L"ucirc",    L'\u00FB'},
  {L"uuml",     L'\u00FC'},
  {L"yacute",   L'\u00FD'},
  {L"thorn",    L'\u00FE'},
  {L"yuml",     L'\u00FF'},

  //////////////////////////////////////////////////////////////////////////////
  // Symbols, mathematical symbols, and Greek letters

  // Latin Extended-B
  {L"fnof",     L'\u0192'},

  // Greek
  {L"Alpha",    L'\u0391'},
  {L"Beta",     L'\u0392'},
  {L"Gamma",    L'\u0393'},
  {L"Delta",    L'\u0394'},
  {L"Epsilon",  L'\u0395'},
  {L"Zeta",     L'\u0396'},
  {L"Eta",      L'\u0397'},
  {L"Theta",    L'\u0398'},
  {L"Iota",     L'\u0399'},
  {L"Kappa",    L'\u039A'},
  {L"Lambda",   L'\u039B'},
  {L"Mu",       L'\u039C'},
  {L"Nu",       L'\u039D'},
  {L"Xi",       L'\u039E'},
  {L"Omicron",  L'\u039F'},
  {L"Pi",       L'\u03A0'},
  {L"Rho",      L'\u03A1'},
  {L"Sigma",    L'\u03A3'},
  {L"Tau",      L'\u03A4'},
  {L"Upsilon",  L'\u03A5'},
  {L"Phi",      L'\u03A6'},
  {L"Chi",      L'\u03A7'},
  {L"Psi",      L'\u03A8'},
  {L"Omega",    L'\u03A9'},
  {L"alpha",    L'\u03B1'},
  {L"beta",     L'\u03B2'},
  {L"gamma",    L'\u03B3'},
  {L"delta",    L'\u03B4'},
  {L"epsilon",  L'\u03B5'},
  {L"zeta",     L'\u03B6'},
  {L"eta",      L'\u03B7'},
  {L"theta",    L'\u03B8'},
  {L"iota",     L'\u03B9'},
  {L"kappa",    L'\u03BA'},
  {L"lambda",   L'\u03BB'},
  {L"mu",       L'\u03BC'},
  {L"nu",       L'\u03BD'},
  {L"xi",       L'\u03BE'},
  {L"omicron",  L'\u03BF'},
  {L"pi",       L'\u03C0'},
  {L"rho",      L'\u03C1'},
  {L"sigmaf",   L'\u03C2'},
  {L"sigma",    L'\u03C3'},
  {L"tau",      L'\u03C4'},
  {L"upsilon",  L'\u03C5'},
  {L"phi",      L'\u03C6'},
  {L"chi",      L'\u03C7'},
  {L"psi",      L'\u03C8'},
  {L"omega",    L'\u03C9'},
  {L"thetasym", L'\u03D1'},
  {L"upsih",    L'\u03D2'},
  {L"piv",      L'\u03D6'},

  // General Punctuation
  {L"bull",     L'\u2022'},
  {L"hellip",   L'\u2026'},
  {L"prime",    L'\u2032'},
  {L"Prime",    L'\u2033'},
  {L"oline",    L'\u203E'},
  {L"frasl",    L'\u2044'},

  // Letterlike Symbols
  {L"weierp",   L'\u2118'},
  {L"image",    L'\u2111'},
  {L"real",     L'\u211C'},
  {L"trade",    L'\u2122'},
  {L"alefsym",  L'\u2135'},

  // Arrows
  {L"larr",     L'\u2190'},
  {L"uarr",     L'\u2191'},
  {L"rarr",     L'\u2192'},
  {L"darr",     L'\u2193'},
  {L"harr",     L'\u2194'},
  {L"crarr",    L'\u21B5'},
  {L"lArr",     L'\u21D0'},
  {L"uArr",     L'\u21D1'},
  {L"rArr",     L'\u21D2'},
  {L"dArr",     L'\u21D3'},
  {L"hArr",     L'\u21D4'},

  // Mathematical Operators
  {L"forall",   L'\u2200'},
  {L"part",     L'\u2202'},
  {L"exist",    L'\u2203'},
  {L"empty",    L'\u2205'},
  {L"nabla",    L'\u2207'},
  {L"isin",     L'\u2208'},
  {L"notin",    L'\u2209'},
  {L"ni",       L'\u220B'},
  {L"prod",     L'\u220F'},
  {L"sum",      L'\u2211'},
  {L"minus",    L'\u2212'},
  {L"lowast",   L'\u2217'},
  {L"radic",    L'\u221A'},
  {L"prop",     L'\u221D'},
  {L"infin",    L'\u221E'},
  {L"ang",      L'\u2220'},
  {L"and",      L'\u2227'},
  {L"or",       L'\u2228'},
  {L"cap",      L'\u2229'},
  {L"cup",      L'\u222A'},
  {L"int",      L'\u222B'},
  {L"there4",   L'\u2234'},
  {L"sim",      L'\u223C'},
  {L"cong",     L'\u2245'},
  {L"asymp",    L'\u2248'},
  {L"ne",       L'\u2260'},
  {L"equiv",    L'\u2261'},
  {L"le",       L'\u2264'},
  {L"ge",       L'\u2265'},
  {L"sub",      L'\u2282'},
  {L"sup",      L'\u2283'},
  {L"nsub",     L'\u2284'},
  {L"sube",     L'\u2286'},
  {L"supe",     L'\u2287'},
  {L"oplus",    L'\u2295'},
  {L"otimes",   L'\u2297'},
  {L"perp",     L'\u22A5'},
  {L"sdot",     L'\u22C5'},

  // Miscellaneous Technical
  {L"lceil",    L'\u2308'},
  {L"rceil",    L'\u2309'},
  {L"lfloor",   L'\u230A'},
  {L"rfloor",   L'\u230B'},
  {L"lang",     L'\u2329'},
  {L"rang",     L'\u232A'},

  // Geometric Shapes
  {L"loz",      L'\u25CA'},

  // Miscellaneous Symbols
  {L"spades",   L'\u2660'},
  {L"clubs",    L'\u2663'},
  {L"hearts",   L'\u2665'},
  {L"diams",    L'\u2666'},

  //////////////////////////////////////////////////////////////////////////////
  // Markup-significant and internationalization characters

  // C0 Controls and Basic Latin
  {L"quot",     L'\"'},
  {L"amp",      L'&'},
  {L"lt",       L'<'},
  {L"gt",       L'>'},

  // Latin Extended-A
  {L"OElig",    L'\u0152'},
  {L"oelig",    L'\u0153'},
  {L"Scaron",   L'\u0160'},
  {L"scaron",   L'\u0161'},
  {L"Yuml",     L'\u0178'},

  // Spacing Modifier Letters
  {L"circ",     L'\u02C6'},
  {L"tilde",    L'\u02DC'},

  // General Punctuation
  {L"ensp",     L'\u2002'},
  {L"emsp",     L'\u2003'},
  {L"thinsp",   L'\u2009'},
  {L"zwnj",     L'\u200C'},
  {L"zwj",      L'\u200D'},
  {L"lrm",      L'\u200E'},
  {L"rlm",      L'\u200F'},
  {L"ndash",    L'\u2013'},
  {L"mdash",    L'\u2014'},
  {L"lsquo",    L'\u2018'},
  {L"rsquo",    L'\u2019'},
  {L"sbquo",    L'\u201A'},
  {L"ldquo",    L'\u201C'},
  {L"rdquo",    L'\u201D'},
  {L"bdquo",    L'\u201E'},
  {L"dagger",   L'\u2020'},
  {L"Dagger",   L'\u2021'},
  {L"permil",   L'\u2030'},
  {L"lsaquo",   L'\u2039'},
  {L"rsaquo",   L'\u203A'},
  {L"euro",     L'\u20AC'},
};

void DecodeHtmlEntities(std::wstring& str) {
  constexpr size_t kMinEntityLength = 2;
  constexpr size_t kMaxEntityLength = 8;

  const auto peek = [&str](const size_t pos) {
    return pos < str.size() ? str[pos] : L'\0';
  };

  using predicate_t = std::function<bool(const wchar_t)>;
  const auto read = [&str](const size_t pos, predicate_t pred) {
    const auto it = std::find_if_not(str.begin() + pos, str.end(), pred);
    if (it != str.end() && *it == L';') {
      const auto count = std::distance(str.begin() + pos, it);
      return std::wstring_view(str.data() + pos, count);
    }
    return std::wstring_view{};
  };
  
  size_t pos = str.find(L'&');
  for (; pos != str.npos; pos = str.find(L'&', ++pos)) {
    if (peek(pos + 1) == L'#') {
      if (peek(pos + 2) == L'x') {
        if (const auto view = read(pos + 3, IsHexadecimalChar); !view.empty()) {
          const auto c = static_cast<wchar_t>(wcstoul(view.data(), nullptr, 16));
          str.replace(pos, 3 + view.size() + 1, 1, c);  // &#x26;
        }
      } else {
        if (const auto view = read(pos + 2, IsNumericChar); !view.empty()) {
          const auto c = static_cast<wchar_t>(ToInt(std::wstring{view}));
          str.replace(pos, 2 + view.size() + 1, 1, c);  // &#38;
        }
      }
    } else {
      const auto view = read(pos + 1, IsAlphanumericChar);
      if (kMinEntityLength <= view.size() && view.size() <= kMaxEntityLength) {
        const auto it = html_entities.find(view);
        if (it != html_entities.end()) {
          str.replace(pos, 1 + view.size() + 1, 1, it->second);  // &amp;
        }
      }
    }
  }
}

void StripHtmlTags(std::wstring& str) {
  int index_begin = -1;
  int index_end = -1;

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
