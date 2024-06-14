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

#include "base/atf.h"

#include "base/string.h"

// @TODO: Replace with https://github.com/erengy/atf when available

namespace atf {

static std::vector<std::wstring> GetFunctionParams(const std::wstring& body) {
  std::vector<std::wstring> params;

  size_t param_begin = 0, param_end = -1;
  do {  // Split by unescaped comma
    do {
      param_end = InStr(body, L",", static_cast<int>(param_end + 1));
    } while (0 < param_end &&
             param_end < body.size() - 1 &&
             body[param_end - 1] == '\\');
    if (param_end == -1)
      param_end = body.size();

    params.push_back(body.substr(param_begin, param_end - param_begin));
    param_begin = param_end + 1;
  } while (param_begin <= body.size());

  return params;
}

static std::wstring EvaluateFunction(const std::wstring& func_name,
                                     const std::wstring& func_body) {
  std::wstring str;

  // Parse parameters
  auto body_parts = GetFunctionParams(func_body);

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

static std::wstring EscapeEntities(const std::wstring& str) {
  std::wstring escaped;
  size_t entity_pos;

  for (size_t pos = 0; pos <= str.length(); ) {
    entity_pos = InStrChars(str, L"$,()%\\", static_cast<int>(pos));
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

static std::wstring UnescapeEntities(const std::wstring& str) {
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

static std::wstring ReplaceFunctions(std::wstring str) {
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
                i = static_cast<int>(str.length());
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

  return str;
}

static std::wstring ReplaceVariables(std::wstring str, const field_map_t& map) {
  int pos_var = 0;
  do {
    pos_var = InStr(str, L"%", pos_var);
    if (pos_var > -1) {
      int pos_end = InStr(str, L"%", pos_var + 1);
      if (pos_end > -1) {
        const auto var = str.substr(pos_var + 1, pos_end - pos_var - 1);
        if (const auto it = map.find(var); it != map.end()) {
          if (it->second) {
            const auto evaluated = EscapeEntities(*it->second);
            str.replace(pos_var, var.size() + 2, evaluated);
            pos_var += static_cast<int>(evaluated.size());
          } else {
            str.erase(pos_var, var.size() + 2);
          }
          continue;
        } else {
          pos_var = pos_end + 1;
        }
      } else {
        pos_var++;
      }
    }
  } while (pos_var > -1);
  return str;
}

std::wstring Replace(std::wstring str, const field_map_t& map) {
  str = ReplaceVariables(str, map);
  ReplaceString(str, L"\\n", L"\n");
  ReplaceString(str, L"\\t", L"\t");
  str = ReplaceFunctions(str);
  str = UnescapeEntities(str);
  while (ReplaceString(str, L"\n\n", L"\n"));
  while (ReplaceString(str, L"  ", L" "));
  Trim(str, L"\t\n\r ");
  return str;
}

}  // namespace atf
