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

#include <map>
#include <string>

#include "types.h"

namespace pugi {
class xml_node;
}

namespace base {

class Setting {
public:
  Setting() {}
  Setting(bool attribute, const std::wstring& path);
  Setting(bool attribute, const std::wstring& default_value, const std::wstring& path);
  ~Setting() {}

  bool attribute;
  std::wstring default_value;
  std::wstring path;
  std::wstring value;
};

class Settings {
public:
  const std::wstring& operator[](enum_t name) const;

  bool GetBool(enum_t name) const;
  int GetInt(enum_t name) const;
  const std::wstring& GetWstr(enum_t name) const;

  void Set(enum_t name, bool value);
  void Set(enum_t name, int value);
  void Set(enum_t name, const std::wstring& value);
  bool Toggle(enum_t name);

protected:
  void InitializeKey(enum_t name, const wchar_t* default_value, const std::wstring& path);
  std::wstring ReadValue(const pugi::xml_node& node_parent, const std::wstring& path,
                         const bool attribute, const std::wstring& default_value);
  void ReadValue(const pugi::xml_node& node_parent, enum_t name);
  void WriteValue(const pugi::xml_node& node_parent, enum_t name);

  virtual void InitializeMap() = 0;

  std::map<enum_t, Setting> map_;
};

}  // namespace base
