/*
** Taiga
** Copyright (C) 2010-2018, Eren Okka
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

#include <set>

#include "base/string.h"
#include "base/url.h"
#include "library/anime_db.h"
#include "library/anime_episode.h"
#include "library/anime_util.h"
#include "sync/anilist_util.h"
#include "sync/kitsu_util.h"
#include "sync/myanimelist_util.h"
#include "sync/sync.h"
#include "taiga/dummy.h"
#include "taiga/script.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "track/media.h"
#include "ui/ui.h"

// The idea behind Taiga's script functions is borrowed from Mp3tag, which
// itself got it from foobar2000. See the following links for more information:
//   http://wiki.hydrogenaudio.org/index.php?title=Foobar2000:Title_Formatting_Reference
//   http://help.mp3tag.de/main_scripting.html

static const std::set<std::wstring> script_functions = {
  L"and",
  L"cut",
  L"equal",
  L"gequal",
  L"greater",
  L"if",
  L"if2",
  L"ifequal",
  L"lequal",
  L"len",
  L"less",
  L"lower",
  L"not",
  L"num",
  L"or",
  L"pad",
  L"replace",
  L"substr",
  L"triml",
  L"trimr",
  L"upper"
};

static const std::set<std::wstring> script_variables = {
  L"animeurl",
  L"audio",
  L"checksum",
  L"episode",
  L"file",
  L"folder",
  L"group",
  L"id",
  L"image",
  L"manual",
  L"name",
  L"playstatus",
  L"resolution",
  L"rewatching",
  L"score",
  L"season",
  L"status",
  L"title",
  L"total",
  L"user",
  L"version",
  L"video",
  L"watched"
};

////////////////////////////////////////////////////////////////////////////////

std::wstring EvaluateFunction(const std::wstring& func_name,
                              const std::wstring& func_body) {
  std::wstring str;

  // Parse parameters
  std::vector<std::wstring> body_parts;
  size_t param_begin = 0, param_end = -1;
  do {  // Split by unescaped comma
    do {
      param_end = InStr(func_body, L",", param_end + 1);
    } while (0 < param_end &&
             param_end < func_body.length() - 1 &&
             func_body[param_end - 1] == '\\');
    if (param_end == -1)
      param_end = func_body.length();

    body_parts.push_back(
        func_body.substr(param_begin, param_end - param_begin));
    param_begin = param_end + 1;
  } while (param_begin <= func_body.length());

  // All functions should have parameters
  if (body_parts.empty())
    return std::wstring();

  // $and(x,y)
  //   Returns true, if all arguments evaluate to true.
  if (func_name == L"and") {
    for (size_t i = 0; i < body_parts.size(); i++)
      if (body_parts[i].empty())
        return std::wstring();
    return L"true";
  // $not(x)
  //   Returns true, if x is false.
  } else if (func_name == L"not") {
    if (body_parts.empty() || body_parts[0].empty())
      return L"true";
  // $or(x,y)
  //   Returns true, if at least one argument evaluates to true.
  } else if (func_name == L"or") {
    for (size_t i = 0; i < body_parts.size(); i++)
      if (!body_parts[i].empty())
        return L"true";

  // $cut(string,len)
  //   Returns first len characters of string.
  } else if (func_name == L"cut") {
    if (body_parts.size() > 1) {
      int length = ToInt(body_parts[1]);
      if (length >= 0 && length < static_cast<int>(body_parts[0].length()))
        body_parts[0].resize(length);
      str = body_parts[0];
    }

  // $equal(x,y)
  //   Returns true, if x is equal to y.
  } else if (func_name == L"equal") {
    if (body_parts.size() > 1) {
      if (IsNumericString(body_parts[0]) && IsNumericString(body_parts[1])) {
        if (ToInt(body_parts[0]) == ToInt(body_parts[1]))
          return L"true";
      } else {
        if (CompareStrings(body_parts[0], body_parts[1]) == 0)
          return L"true";
      }
    }
  // $gequal(x,y)
  //   Returns true, if x is greater as or equal to y.
  } else if (func_name == L"gequal") {
    if (body_parts.size() > 1) {
      if (IsNumericString(body_parts[0]) && IsNumericString(body_parts[1])) {
        if (ToInt(body_parts[0]) >= ToInt(body_parts[1]))
          return L"true";
      } else {
        if (CompareStrings(body_parts[0], body_parts[1]) >= 0)
          return L"true";
      }
    }
  // $greater(x,y)
  //   Returns true, if x is greater than y.
  } else if (func_name == L"greater") {
    if (body_parts.size() > 1) {
      if (IsNumericString(body_parts[0]) && IsNumericString(body_parts[1])) {
        if (ToInt(body_parts[0]) > ToInt(body_parts[1]))
          return L"true";
      } else {
        if (CompareStrings(body_parts[0], body_parts[1]) > 0)
          return L"true";
      }
    }
  // $lequal(x,y)
  //   Returns true, if x is less than or equal to y.
  } else if (func_name == L"lequal") {
    if (body_parts.size() > 1) {
      if (IsNumericString(body_parts[0]) && IsNumericString(body_parts[1])) {
        if (ToInt(body_parts[0]) <= ToInt(body_parts[1]))
          return L"true";
      } else {
        if (CompareStrings(body_parts[0], body_parts[1]) <= 0)
          return L"true";
      }
    }
  // $less(x,y)
  //   Returns true, if x is less than y.
  } else if (func_name == L"less") {
    if (body_parts.size() > 1) {
      if (IsNumericString(body_parts[0]) && IsNumericString(body_parts[1])) {
        if (ToInt(body_parts[0]) < ToInt(body_parts[1]))
          return L"true";
      } else {
        if (CompareStrings(body_parts[0], body_parts[1]) < 0)
          return L"true";
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
        if (!body_parts[0].empty())
          str = body_parts[1];
        break;
      // $if(cond,then,else)
      case 3:
        str = !body_parts[0].empty() ? body_parts[1] : body_parts[2];
        break;
    }
  // $if2(a,else)
  } else if (func_name == L"if2") {
    if (body_parts.size() > 1)
      str = !body_parts[0].empty() ? body_parts[0] : body_parts[1];
  // $ifequal()
  } else if (func_name == L"ifequal") {
    switch (body_parts.size()) {
      // $ifequal(n1,n2,then)
      case 3:
        if (body_parts[0] == body_parts[1])
          str = body_parts[2];
        break;
      // $ifequal(n1,n2,then,else)
      case 4:
        str = body_parts[0] == body_parts[1] ? body_parts[2] : body_parts[3];
        break;
    }

  // $len(string)
  //   Returns length of string in characters.
  } else if (func_name == L"len") {
    str = ToWstr(body_parts[0].length());

  // $lower(string)
  //   Converts string to lowercase.
  } else if (func_name == L"lower") {
    str = ToLower_Copy(body_parts[0]);
  // $upper(string)
  //   Converts string to uppercase.
  } else if (func_name == L"upper") {
    str = ToUpper_Copy(body_parts[0]);

  // $num(n,len)
  //   Formats the integer number n in decimal notation with len characters.
  //   Pads with zeros from the left if necessary.
  } else if (func_name == L"num") {
    if (body_parts.size() > 1) {
      int length = ToInt(body_parts[1]);
      if (length > static_cast<int>(body_parts[0].length()))
        str.append(length - body_parts[0].length(), '0');
    }
    str += body_parts[0];
  // $pad(s,len,chars)
  //   Pads string from the left with chars to len characters.
  //   If length of chars is smaller than len, padding will repeat.
  } else if (func_name == L"pad") {
    if (body_parts.size() == 2)
      body_parts.push_back(L" ");
    if (body_parts.size() > 2) {
      if (body_parts[2].empty())
        body_parts[2] = L" ";
      int length = ToInt(body_parts[1]);
      if (length > static_cast<int>(body_parts[0].length()))
        for (size_t i = 0; i < length - body_parts[0].length(); i++)
          str += body_parts[2].at(i % body_parts[2].length());
    }
    str += body_parts[0];

  // $replace(a,b,c)
  //   Replaces all occurrences of string b in string a with string c.
  } else if (func_name == L"replace") {
    if (body_parts.size() == 2) body_parts.push_back(L"");
    if (body_parts.size() > 2) {
      str = body_parts[0];
      while (ReplaceString(str, body_parts[1], body_parts[2]));
    }

  // $substr(s,pos,n)
  //   Returns substring of string s, starting from pos with a length of n characters.
  } else if (func_name == L"substr") {
    if (body_parts.size() > 2)
      if (ToInt(body_parts[1]) <= static_cast<int>(body_parts[0].length()))
        str = body_parts[0].substr(ToInt(body_parts[1]), ToInt(body_parts[2]));

  // $triml()
  //   Removes leading characters from string.
  } else if (func_name == L"triml") {
    // $triml(s,c)
    if (body_parts.size() > 1) {
      TrimLeft(body_parts[0], body_parts[1].c_str());
    // $triml(s)
    } else {
      TrimLeft(body_parts[0]);
    }
  // $trimr()
  //   Removes trailing characters from string.
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

////////////////////////////////////////////////////////////////////////////////

bool IsScriptFunction(const std::wstring& str) {
  return script_functions.count(str) > 0;
}

bool IsScriptVariable(const std::wstring& str) {
  return script_variables.count(str) > 0;
}

////////////////////////////////////////////////////////////////////////////////

std::wstring ReplaceVariables(std::wstring str, const anime::Episode& episode,
                              bool url_encode, bool is_manual, bool is_preview) {
  auto anime_item = AnimeDatabase.FindItem(episode.anime_id);
  if (!anime_item && is_preview)
    anime_item = &taiga::DummyAnime;

  std::wstring id;
  if (anime_item)
    id = anime_item->GetId(taiga::GetCurrentServiceId());

  #define VALIDATE(x, y) \
      anime_item ? x : y
  #define ENCODE(x) \
      url_encode ? EscapeScriptEntities(EncodeUrl(x)) : EscapeScriptEntities(x)
  #define REPLACE(x, y) \
      if (var == x) { \
        str.replace(pos_var, var.length() + 2, y); \
        pos_var += static_cast<int>(std::wstring(y).length()); \
        continue; \
      }

  // Prepare episode value
  std::wstring episode_number = ToWstr(anime::GetEpisodeHigh(episode));
  TrimLeft(episode_number, L"0");

  // Prepare folder
  std::wstring folder = episode.folder;
  TrimRight(folder, L"\\");

  // Replace variables
  int pos_var = 0;
  do {
    pos_var = InStr(str, L"%", pos_var);
    if (pos_var > -1) {
      int pos_end = InStr(str, L"%", pos_var + 1);
      if (pos_end > -1) {
        std::wstring var = str.substr(pos_var + 1, pos_end - pos_var - 1);
        if (IsScriptVariable(var)) {
          REPLACE(L"title", VALIDATE(ENCODE(anime::GetPreferredTitle(*anime_item)), ENCODE(episode.anime_title())));
          REPLACE(L"watched", VALIDATE(ENCODE(anime::TranslateNumber(anime_item->GetMyLastWatchedEpisode(), L"")), L""));
          REPLACE(L"total", VALIDATE(ENCODE(anime::TranslateNumber(anime_item->GetEpisodeCount(), L"")), L""));
          REPLACE(L"score", VALIDATE(ENCODE(anime::TranslateMyScore(anime_item->GetMyScore(), L"")), L""));
          REPLACE(L"season", VALIDATE(ENCODE(anime::TranslateDateToSeasonString(anime_item->GetDateStart())), L""));
          REPLACE(L"id", ENCODE(id));
          REPLACE(L"image", VALIDATE(ENCODE(anime_item->GetImageUrl()), L""));
          REPLACE(L"status", VALIDATE(ENCODE(ToWstr(anime_item->GetMyStatus())), L""));
          REPLACE(L"rewatching", VALIDATE(ENCODE(ToWstr(anime_item->GetMyRewatching())), L""));
          REPLACE(L"name", ENCODE(episode.episode_title()));
          REPLACE(L"episode", ENCODE(episode_number));
          REPLACE(L"version", ENCODE(ToWstr(episode.release_version())));
          REPLACE(L"group", ENCODE(episode.release_group()));
          REPLACE(L"resolution", ENCODE(episode.video_resolution()));
          REPLACE(L"video", ENCODE(episode.video_terms()));
          REPLACE(L"audio", ENCODE(episode.audio_terms()));
          REPLACE(L"checksum", ENCODE(episode.file_checksum()));
          REPLACE(L"file", ENCODE(episode.file_name_with_extension()));
          REPLACE(L"folder", ENCODE(folder));
          REPLACE(L"user", ENCODE(taiga::GetCurrentUsername()));
          REPLACE(L"manual", is_manual ? L"true" : L"");
          switch (MediaPlayers.play_status) {
            case track::recognition::PlayStatus::Stopped: REPLACE(L"playstatus", L"stopped"); break;
            case track::recognition::PlayStatus::Playing: REPLACE(L"playstatus", L"playing"); break;
            case track::recognition::PlayStatus::Updated: REPLACE(L"playstatus", L"updated"); break;
          }
          switch (taiga::GetCurrentServiceId()) {
            case sync::kMyAnimeList:
              REPLACE(L"animeurl", ENCODE(sync::myanimelist::GetAnimePage(*anime_item)));
              break;
            case sync::kKitsu:
              REPLACE(L"animeurl", ENCODE(sync::kitsu::GetAnimePage(*anime_item)));
              break;
            case sync::kAniList:
              REPLACE(L"animeurl", ENCODE(sync::anilist::GetAnimePage(*anime_item)));
              break;
          }
        } else {
          pos_var = pos_end + 1;
        }
      } else {
        pos_var++;
      }
    }
  } while (pos_var > -1);

  #undef REPLACE
  #undef ENCODE
  #undef VALIDATE

  // Replace special characters
  ReplaceString(str, L"\\n", L"\n");
  ReplaceString(str, L"\\t", L"\t");

  // Scripting
  int pos_func = 0, pos_left = 0, pos_right = 0;
  int open_brackets = 0;
  do {
    // Find non-escaped dollar sign
    pos_func = 0;
    while (true) {
      pos_func = InStr(str, L"$", pos_func);
      if (pos_func > 0 && str[pos_func - 1] == '\\') {
        pos_func += 1;
      } else {
        break;
      }
    }

    if (pos_func > -1) {
      for (unsigned int i = pos_func; i < str.length(); i++) {
        switch (str[i]) {
          case '$':
            pos_func = i;
            pos_left = pos_right = open_brackets = 0;
            break;
          case '(':
            if (pos_func > -1) {
              if (!open_brackets++)
                pos_left = i;
              pos_right = 0;
            }
            break;
          case ')':
            if (pos_left) {
              if (open_brackets == 1) {
                pos_right = i;
                std::wstring func_name =
                    str.substr(pos_func + 1, pos_left - (pos_func + 1));
                std::wstring func_body =
                    str.substr(pos_left + 1, pos_right - (pos_left + 1));
                str = str.substr(0, pos_func) +
                      str.substr(pos_right + 1, str.length() - (pos_right + 1));
                str.insert(pos_func, EvaluateFunction(func_name, func_body));
                i = str.length();
              }
              if (open_brackets > 0)
                open_brackets--;
            }
            break;
          case '\\':
            i++;
            break;
        }
      }
      if (!pos_left || !pos_right)
        break;
    }
  } while (pos_func > -1);

  // Unescape
  str = UnescapeScriptEntities(str);

  // Clean-up
  while (ReplaceString(str, L"\n\n", L"\n"));
  while (ReplaceString(str, L"  ", L" "));
  Trim(str, L"\t\n\r ");

  // Return
  return str;
}

std::wstring EscapeScriptEntities(const std::wstring& str) {
  std::wstring escaped;
  size_t entity_pos;

  for (size_t pos = 0; pos <= str.length(); ) {
    entity_pos = InStrChars(str, L"$,()%\\", pos);
    if (entity_pos != -1) {
      escaped.append(str, pos, entity_pos - pos);
      escaped.append(L"\\");
      escaped.append(str, entity_pos, 1);
    } else {
      entity_pos = str.length();
      escaped.append(str, pos, entity_pos - pos);
    }
    pos = entity_pos + 1;
  }

  return escaped;
}

std::wstring UnescapeScriptEntities(const std::wstring& str) {
  std::wstring unescaped;
  unescaped.reserve(str.size());

  for (auto it = str.begin(); it != str.end(); ++it) {
    switch (*it) {
      case '\\': {
        auto next = std::next(it);
        if (next != str.end() && *next == '\\')
          unescaped.push_back(*next);
        break;
      }
      default:
        unescaped += *it;
        break;
    }
  }

  return unescaped;
}