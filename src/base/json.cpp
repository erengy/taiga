/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#include "json.h"
#include "string.h"

bool JsonParseString(const std::string& str, Json& output) {
  try {
    output = Json::parse(str.begin(), str.end());
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

bool JsonParseString(const std::wstring& str, Json& output) {
  return JsonParseString(WstrToStr(str), output);
}

////////////////////////////////////////////////////////////////////////////////

bool JsonReadBool(const Json& json, const std::string& key) {
  const auto it = json.find(key);
  return it != json.end() && it->is_boolean() ? it->get<bool>() : false;
}

double JsonReadDouble(const Json& json, const std::string& key) {
  const auto it = json.find(key);
  return it != json.end() && it->is_number_float() ? it->get<double>() : 0.0;
}

int JsonReadInt(const Json& json, const std::string& key) {
  const auto it = json.find(key);
  return it != json.end() && it->is_number_integer() ? it->get<int>() : 0;
}

std::string JsonReadStr(const Json& json, const std::string& key) {
  const auto it = json.find(key);
  return it != json.end() && it->is_string() ? it->get<std::string>() : std::string();
}
