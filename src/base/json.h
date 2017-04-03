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

#pragma once

#include <string>

#include <json/src/json.hpp>

using Json = nlohmann::json;

bool JsonParseString(const std::string& str, Json& output);
bool JsonParseString(const std::wstring& str, Json& output);

bool JsonReadBool(const Json& json, const std::string& key);
double JsonReadDouble(const Json& json, const std::string& key);
int JsonReadInt(const Json& json, const std::string& key);
std::string JsonReadStr(const Json& json, const std::string& key);
