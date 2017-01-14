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
#include <vector>

#include <pugixml/src/pugixml.hpp>

using pugi::xml_document;
using pugi::xml_node;
using pugi::xml_parse_result;

#define foreach_xmlnode_(node, parent, name) \
    for (pugi::xml_node node = parent.child(name); node; \
         node = node.next_sibling(name))

std::wstring XmlGetNodeAsString(pugi::xml_node node);

int XmlReadIntValue(pugi::xml_node& node, const wchar_t* name);
std::wstring XmlReadStrValue(pugi::xml_node& node, const wchar_t* name);

void XmlReadChildNodes(pugi::xml_node& parent_node,
                       std::vector<std::wstring>& output,
                       const wchar_t* name);
void XmlWriteChildNodes(pugi::xml_node& parent_node,
                        const std::vector<std::wstring>& input,
                        const wchar_t* name,
                        pugi::xml_node_type node_type = pugi::node_pcdata);

void XmlWriteIntValue(pugi::xml_node& node, const wchar_t* name, int value);
void XmlWriteStrValue(pugi::xml_node& node, const wchar_t* name,
                      const wchar_t* value,
                      pugi::xml_node_type node_type = pugi::node_pcdata);

bool XmlWriteDocumentToFile(const pugi::xml_document& document,
                            const std::wstring& path);
