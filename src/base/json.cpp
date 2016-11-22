/*
** Taiga
** Copyright (C) 2010-2016, Eren Okka
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

bool JsonReadArray(const Json::Value& root, const std::string& name,
                   std::vector<std::wstring>& output) {
  size_t previous_size = output.size();

  auto& value = root[name.c_str()];

  for (size_t i = 0; i < value.size(); i++)
    output.push_back(StrToWstr(value[i].asString()));

  return output.size() > previous_size;
}