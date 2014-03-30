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

#include <map>
#include <string>

#include "html.h"
#include "string.h"

std::map<std::wstring, wchar_t> html_entities;

void DecodeHtmlEntities(std::wstring& str) {
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

  if (InStr(str, L"&") == -1)
    return;

  size_t pos = 0;
  size_t reference_pos = 0;
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
            character_value = wcstoul(str.substr(pos, i - pos).c_str(),
                                      nullptr, 16);
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
            std::wstring entity_name = str.substr(pos, length);
            if (html_entities.find(entity_name) != html_entities.end()) {
              character_value = html_entities[entity_name];
            }
          }
        }
      }

      if (character_value <= 0xFFFD) {
        str.replace(reference_pos, i - reference_pos + 1,
                    std::wstring(1, static_cast<wchar_t>(character_value)));
        i = reference_pos - 1;
      } else {
        i = reference_pos;
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