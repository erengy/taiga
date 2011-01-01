/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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

#ifndef XML_H
#define XML_H

#include "std.h"
#include "third_party\pugixml\pugixml.hpp"

using namespace pugi;

// =============================================================================

int XML_ReadIntValue(xml_node& node, const wchar_t* name);
wstring XML_ReadStrValue(xml_node& node, const wchar_t* name);
void XML_ReadChildNodes(xml_node& parent_node, vector<wstring>& str_vector, const wchar_t* name);
void XML_WriteChildNodes(xml_node& parent_node, vector<wstring>& str_vector, const wchar_t* name);
void XML_WriteIntValue(xml_node& node, const wchar_t* name, int value);
void XML_WriteStrValue(xml_node& node, const wchar_t* name, const wchar_t* value, xml_node_type node_type = node_pcdata);

#endif // XML_H