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
#include "animelist.h"
#include "common.h"
#include "myanimelist.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"

#define SCRIPT_FUNCTION_COUNT 17
const wchar_t* script_functions[] = {
  L"cut", 
  L"equal", 
  L"gequal", 
  L"greater", 
  L"if", 
  L"ifequal", 
  L"lequal", 
  L"len", 
  L"less", 
  L"lower", 
  L"num", 
  L"pad", 
  L"replace", 
  L"substr", 
  L"triml", 
  L"trimr", 
  L"upper"
};

#define SCRIPT_VARIABLE_COUNT 20
const wchar_t* script_variables[] = {
  L"audio", 
  L"checksum", 
  L"episode", 
  L"extra", 
  L"file", 
  L"folder", 
  L"group", 
  L"id", 
  L"image", 
  L"name", 
  L"playstatus", 
  L"resolution", 
  L"score", 
  L"status", 
  L"title", 
  L"total", 
  L"user", 
  L"version", 
  L"video", 
  L"watched"
};

// =============================================================================

wstring EvaluateFunction(const wstring& func_name, const wstring& func_body) {
  wstring str;
  vector<wstring> body_parts;
  Split(func_body, L",", body_parts);
  if (body_parts.empty()) return L"";
  
  // $cut(string,len)
  // Returns first len characters on the left of string. 
  if (func_name == L"cut") {
    if (body_parts.size() > 1) {
      int length = ToINT(body_parts[1]);
      if (length >= 0 && length < static_cast<int>(body_parts[0].length())) {
        body_parts[0].resize(length);
      }
      str = body_parts[0];
    }

  // $equal(x,y)
  // Returns true, if x is equal to y.
  } else if (func_name == L"equal") {
    if (body_parts.size() > 1) {
      if (IsNumeric(body_parts[0]) && IsNumeric(body_parts[1])) {
        if (ToINT(body_parts[0]) == ToINT(body_parts[1])) return L"true";
      } else {
        if (CompareStrings(body_parts[0], body_parts[1]) == 0) return L"true";
      }
    }
  // $gequal(x,y)
  // Returns true, if x is greater as or equal to y.
  } else if (func_name == L"gequal") {
    if (body_parts.size() > 1) {
      if (IsNumeric(body_parts[0]) && IsNumeric(body_parts[1])) {
        if (ToINT(body_parts[0]) >= ToINT(body_parts[1])) return L"true";
      } else {
        if (CompareStrings(body_parts[0], body_parts[1]) >= 0) return L"true";
      }
    }
  // $greater(x,y)
  // Returns true, if x is greater than y.
  } else if (func_name == L"greater") {
    if (body_parts.size() > 1) {
      if (IsNumeric(body_parts[0]) && IsNumeric(body_parts[1])) {
        if (ToINT(body_parts[0]) > ToINT(body_parts[1])) return L"true";
      } else {
        if (CompareStrings(body_parts[0], body_parts[1]) > 0) return L"true";
      }
    }
  // $lequal(x,y)
  // // Returns true, if x is less than or equal to y.
  } else if (func_name == L"lequal") {
    if (body_parts.size() > 1) {
      if (IsNumeric(body_parts[0]) && IsNumeric(body_parts[1])) {
        if (ToINT(body_parts[0]) <= ToINT(body_parts[1])) return L"true";
      } else {
        if (CompareStrings(body_parts[0], body_parts[1]) <= 0) return L"true";
      }
    }
  // $less(x,y)
  // Returns true, if x is less than y.
  } else if (func_name == L"less") {
    if (body_parts.size() > 1) {
      if (IsNumeric(body_parts[0]) && IsNumeric(body_parts[1])) {
        if (ToINT(body_parts[0]) < ToINT(body_parts[1])) return L"true";
      } else {
        if (CompareStrings(body_parts[0], body_parts[1]) < 0) return L"true";
      }
    }
  
  // $if()
  } else if (func_name == L"if") {
    switch (body_parts.size()) {
      // $if(cond)      
      case 1:
        str = body_parts[0];
        break;
      // $if(cond,then)
      case 2:
        if (!body_parts[0].empty()) str = body_parts[1];
        break;
      // $if(cond,then,else)
      case 3:
        str = !body_parts[0].empty() ? body_parts[1] : body_parts[2];
        break;
    }
  
  // $ifequal()
  } else if (func_name == L"ifequal") {
    switch (body_parts.size()) {
      // $ifequal(n1,n2,then)
      case 3:
        if (body_parts[0] == body_parts[1]) str = body_parts[2];
        break;
      // $ifequal(n1,n2,then,else)
      case 4:
        str = body_parts[0] == body_parts[1] ? body_parts[2] : body_parts[3];
        break;
    }

  // $len(string)
  // Returns length of string in characters.
  } else if (func_name == L"len") {
    str = ToWSTR(static_cast<int>(body_parts[0].length()));

  // $lower(string)
  // Converts string to lowercase.
  } else if (func_name == L"lower") {
    str = ToLower_Copy(body_parts[0]);
  // $upper(string)
  // Converts string to uppercase.
  } else if (func_name == L"upper") {
    str = ToUpper_Copy(body_parts[0]);

  // $num(n,len)
  // Formats the integer number n in decimal notation with len characters.
  // Pads with zeros from the left if necessary.
  } else if (func_name == L"num") {
    if (body_parts.size() > 1) {
      int length = ToINT(body_parts[1]);
      if (length > static_cast<int>(body_parts[0].length())) {
        str.append(length - body_parts[0].length(), '0');
      }
    }
    str += body_parts[0];
  // $pad(s,len,chars)
  // Pads string from the left with chars to len characters.
  // If length of chars is smaller than len, padding will repeat.
  } else if (func_name == L"pad") {
    if (body_parts.size() == 2) body_parts.push_back(L" ");
    if (body_parts.size() > 2) {
      if (body_parts[2].empty()) body_parts[2] = L" ";
      int length = ToINT(body_parts[1]);
      if (length > static_cast<int>(body_parts[0].length())) {
        for (size_t i = 0; i < length - body_parts[0].length(); i++) {
          str += body_parts[2].at(i % body_parts[2].length());
        }
      }
    }
    str += body_parts[0];

  // $replace(a,b,c)
  // Replaces all occurrences of string b in string a with string c.
  } else if (func_name == L"replace") {
    if (body_parts.size() == 2) body_parts.push_back(L"");
    if (body_parts.size() > 2) {
      str = body_parts[0];
      Replace(str, body_parts[1], body_parts[2], true);
    }

  // $substr(s,pos,n)
  // Returns substring of string s, starting from pos with a length of n characters.
  } else if (func_name == L"substr") {
    if (body_parts.size() > 2) {
      if (ToINT(body_parts[1]) <= static_cast<int>(body_parts[0].length())) {
        str = body_parts[0].substr(ToINT(body_parts[1]), ToINT(body_parts[2]));
      }
    }
    
  // $triml()
  // Removes leading characters from string.
  } else if (func_name == L"triml") {
    // $triml(s,c)
    if (body_parts.size() > 1) {
      TrimLeft(body_parts[0], body_parts[1].c_str());
    // $triml(s)
    } else {
      TrimLeft(body_parts[0]);
    }
  // $trimr()
  // Removes trailing characters from string.
  } else if (func_name == L"trimr") {
    // $trimr(s,c)
    if (body_parts.size() > 1) {
      TrimRight(body_parts[0], body_parts[1].c_str());
    // $trimr(s)
    } else {
      TrimRight(body_parts[0]);
    }
  }

  return str;
}

// =============================================================================

bool IsScriptFunction(const wstring& str) {
  for (int i = 0; i < SCRIPT_FUNCTION_COUNT; i++) {
    if (str == script_functions[i]) return true;
  }
  return false;
}

bool IsScriptVariable(const wstring& str) {
  for (int i = 0; i < SCRIPT_VARIABLE_COUNT; i++) {
    if (str == script_variables[i]) return true;
  }
  return false;
}

// =============================================================================

wstring ReplaceVariables(wstring str, const CEpisode& episode, bool url_encode) {
  bool is_valid = episode.Index > -1 && episode.Index <= AnimeList.Count;
  #define VALIDATE(x, y) is_valid ? x : y
  #define ENCODE(x) url_encode ? EncodeURL(x) : x

  // Replace variables
  Replace(str, L"%title%",   VALIDATE(ENCODE(AnimeList.Item[episode.Index].Series_Title), ENCODE(episode.Title)));
  Replace(str, L"%watched%", VALIDATE(ENCODE(MAL.TranslateNumber(AnimeList.Item[episode.Index].GetLastWatchedEpisode(), L"")), L""));
  Replace(str, L"%total%",   VALIDATE(ENCODE(MAL.TranslateNumber(AnimeList.Item[episode.Index].Series_Episodes, L"")), L""));
  Replace(str, L"%score%",   VALIDATE(ENCODE(MAL.TranslateNumber(AnimeList.Item[episode.Index].GetScore(), L"")), L""));
  Replace(str, L"%id%",      VALIDATE(ENCODE(ToWSTR(AnimeList.Item[episode.Index].Series_ID)), L""));
  Replace(str, L"%image%",   VALIDATE(ENCODE(AnimeList.Item[episode.Index].Series_Image), L""));
  Replace(str, L"%status%",  VALIDATE(ENCODE(ToWSTR(AnimeList.Item[episode.Index].GetStatus())), L""));
  Replace(str, L"%name%",       ENCODE(episode.Name));
  Replace(str, L"%episode%",    ENCODE(GetLastEpisode(episode.Number)));
  Replace(str, L"%version%",    ENCODE(episode.Version));
  Replace(str, L"%group%",      ENCODE(episode.Group));
  Replace(str, L"%resolution%", ENCODE(episode.Resolution));
  Replace(str, L"%video%",      ENCODE(episode.VideoType));
  Replace(str, L"%audio%",      ENCODE(episode.AudioType));
  Replace(str, L"%checksum%",   ENCODE(episode.Checksum));
  Replace(str, L"%extra%",      ENCODE(episode.Extra));
  Replace(str, L"%file%",       ENCODE(episode.File));
  Replace(str, L"%folder%",     ENCODE(episode.Folder));
  Replace(str, L"%user%",       ENCODE(Settings.Account.MAL.User));
  switch (Taiga.PlayStatus) {
    case PLAYSTATUS_STOPPED:
      Replace(str, L"%playstatus%", L"stopped");
      break;
    case PLAYSTATUS_PLAYING:
      Replace(str, L"%playstatus%", L"playing");
      break;
    case PLAYSTATUS_UPDATED:
      Replace(str, L"%playstatus%", L"updated");
      break;
  }

  // Replace special characters
  Replace(str, L"\\n", L"\n", true);
  Replace(str, L"\\t", L"\t", true);
  
  // Scripting
  int pos_func = -1, pos_left = 0, pos_right = 0;
  int open_brackets = 0;
  do {
    pos_func = InStr(str, L"$", 0);
    if (pos_func > -1) {
      for (unsigned int i = pos_func; i < str.length(); i++) {
        switch (str[i]) {
          case '$':
            pos_func = i;
            pos_left = pos_right = open_brackets = 0;
            break;
          case '(':
            if (pos_func > -1) {
              if (!open_brackets++) pos_left = i;
              pos_right = 0;
            }
            break;
          case ')':
            if (pos_left) {
              if (open_brackets == 1) {
                pos_right = i;
                wstring func_name = str.substr(pos_func + 1, pos_left - (pos_func + 1));
                wstring func_body = str.substr(pos_left + 1, pos_right - (pos_left + 1));
                str = str.substr(0, pos_func) + str.substr(pos_right + 1, str.length() - (pos_right + 1));
                str.insert(pos_func, EvaluateFunction(func_name, func_body));
                i = str.length();
              }
              if (open_brackets > 0) open_brackets--;
            }
            break;
        }
      }
      if (!pos_left || !pos_right) break;
    }
  } while (pos_func > -1);
  

  // Clean-up
  Replace(str, L"\n\n", L"\n", true);
  Replace(str, L"  ", L" ", true);

  // Return
  return str;
}