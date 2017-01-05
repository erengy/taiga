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

#include "file.h"
#include "string.h"
#include "xml.h"

struct xml_string_writer: pugi::xml_writer {
  std::string result;

  virtual void write(const void* data, size_t size) {
    result += std::string(static_cast<const char*>(data), size);
  }
};

std::wstring XmlGetNodeAsString(pugi::xml_node node) {
  xml_string_writer writer;
  node.print(writer);

  return StrToWstr(writer.result);
}

int XmlReadIntValue(pugi::xml_node& node, const wchar_t* name) {
  return _wtoi(node.child_value(name));
}

std::wstring XmlReadStrValue(pugi::xml_node& node, const wchar_t* name) {
  return node.child_value(name);
}

void XmlReadChildNodes(pugi::xml_node& parent_node,
                       std::vector<std::wstring>& output,
                       const wchar_t* name) {
  foreach_xmlnode_(child_node, parent_node, name) {
    output.push_back(child_node.child_value());
  }
}

void XmlWriteChildNodes(pugi::xml_node& parent_node,
                        const std::vector<std::wstring>& input,
                        const wchar_t* name,
                        pugi::xml_node_type node_type) {
  for (const auto& value : input) {
    xml_node child_node = parent_node.append_child(name);
    child_node.append_child(node_type).set_value(value.c_str());
  }
}

void XmlWriteIntValue(pugi::xml_node& node, const wchar_t* name, int value) {
  xml_node child = node.append_child(name);
  child.append_child(pugi::node_pcdata).set_value(ToWstr(value).c_str());
}

void XmlWriteStrValue(pugi::xml_node& node, const wchar_t* name,
                      const wchar_t* value, pugi::xml_node_type node_type) {
  xml_node child = node.append_child(name);
  child.append_child(node_type).set_value(value);
}

bool XmlWriteDocumentToFile(const pugi::xml_document& document,
                            const std::wstring& path) {
  CreateFolder(GetPathOnly(path));

  const pugi::char_t* indent = L"\x09";  // horizontal tab
  unsigned int flags = pugi::format_default | pugi::format_write_bom;
  return document.save_file(path.c_str(), indent, flags);
}