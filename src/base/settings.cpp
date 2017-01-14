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

#include "settings.h"
#include "string.h"
#include "xml.h"

namespace base {

Setting::Setting(bool attribute,
                 const std::wstring& path)
    : attribute(attribute),
      path(path) {
}

Setting::Setting(bool attribute,
                 const std::wstring& default_value,
                 const std::wstring& path)
    : attribute(attribute),
      default_value(default_value),
      path(path) {
}

////////////////////////////////////////////////////////////////////////////////

const std::wstring& Settings::operator[](enum_t name) const {
  return GetWstr(name);
}

bool Settings::GetBool(enum_t name) const {
  auto it = map_.find(name);

  if (it != map_.end())
    return ToBool(it->second.value);

  return false;
}

int Settings::GetInt(enum_t name) const {
  auto it = map_.find(name);

  if (it != map_.end())
    return ToInt(it->second.value);

  return 0;
}

const std::wstring& Settings::GetWstr(enum_t name) const {
  auto it = map_.find(name);

  if (it != map_.end())
    return it->second.value;

  return EmptyString();
}

void Settings::Set(enum_t name, bool value) {
  map_[name].value = value ? L"true" : L"false";
}

void Settings::Set(enum_t name, int value) {
  map_[name].value = ToWstr(value);
}

void Settings::Set(enum_t name, const std::wstring& value) {
  map_[name].value = value;
}

bool Settings::Toggle(enum_t name) {
  bool value = !GetBool(name);
  Set(name, value);
  return value;
}

////////////////////////////////////////////////////////////////////////////////

void Settings::InitializeKey(enum_t name, const wchar_t* default_value,
                             const std::wstring& path) {
  if (default_value) {
    map_.insert(std::make_pair(name, base::Setting(true, default_value, path)));
  } else {
    map_.insert(std::make_pair(name, base::Setting(true, path)));
  }
}

std::wstring Settings::ReadValue(const xml_node& node_parent,
                                 const std::wstring& path,
                                 const bool attribute,
                                 const std::wstring& default_value) {
  std::vector<std::wstring> node_names;
  Split(path, L"/", node_names);

  const wchar_t* node_name = node_names.back().c_str();

  xml_node current_node = node_parent;
  for (int i = 0; i < static_cast<int>(node_names.size()) - 1; i++) {
    current_node = current_node.child(node_names.at(i).c_str());
  }

  if (attribute) {
    return current_node.attribute(node_name).as_string(default_value.c_str());
  } else {
    return XmlReadStrValue(current_node, node_name);
  }
}

void Settings::ReadValue(const xml_node& node_parent, enum_t name) {
  Setting& item = map_[name];
  item.value = ReadValue(node_parent, item.path,
                         item.attribute, item.default_value);
}

void Settings::WriteValue(const xml_node& node_parent, enum_t name) {
  Setting& item = map_[name];

  std::vector<std::wstring> node_names;
  Split(item.path, L"/", node_names);

  const wchar_t* node_name = node_names.back().c_str();

  xml_node current_node = node_parent;
  for (int i = 0; i < static_cast<int>(node_names.size()) - 1; i++) {
    std::wstring child_name = node_names.at(i);
    if (!current_node.child(child_name.c_str())) {
      current_node = current_node.append_child(child_name.c_str());
    } else {
      current_node = current_node.child(child_name.c_str());
    }
  }

  if (item.attribute) {
    current_node.append_attribute(node_name) = item.value.c_str();
  } else {
    XmlWriteStrValue(current_node, node_name, item.value.c_str());
  }
}

}  // namespace base