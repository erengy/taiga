/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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
#include "string.h"
#include "xml.h"

// =============================================================================

int XML_ReadIntValue(xml_node& node, const wchar_t* name) {
  return _wtoi(node.child_value(name));
}

wstring XML_ReadStrValue(xml_node& node, const wchar_t* name) {
  return node.child_value(name);
}

void XML_ReadChildNodes(xml_node& parent_node, vector<wstring>& str_vector, const wchar_t* name) {
  for (xml_node child_node = parent_node.child(name); child_node; child_node = child_node.next_sibling(name)) {
    str_vector.push_back(child_node.child_value());
  }
}

void XML_WriteChildNodes(xml_node& parent_node, vector<wstring>& str_vector, const wchar_t* name) {
  for (size_t i = 0; i < str_vector.size(); i++) {
    xml_node child_node = parent_node.append_child();
    child_node.set_name(name);
    child_node.append_child(node_pcdata).set_value(str_vector[i].c_str());
  }
}

void XML_WriteIntValue(xml_node& node, const wchar_t* name, int value) {
  xml_node child = node.append_child();
  child.set_name(name);
  child.append_child(node_pcdata).set_value(WSTR(value).c_str());
}

void XML_WriteStrValue(xml_node& node, const wchar_t* name, const wchar_t* value, xml_node_type node_type) {
  xml_node child = node.append_child();
  child.set_name(name);
  child.append_child(node_type).set_value(value);
}