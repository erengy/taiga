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

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <pugixml.hpp>

using XmlAttribute = pugi::xml_attribute;
using XmlDocument = pugi::xml_document;
using XmlNode = pugi::xml_node;

XmlAttribute XmlAttr(XmlNode& node, const std::wstring_view name);
XmlNode XmlChild(XmlNode& node, const std::wstring_view name);

std::wstring XmlDump(const XmlNode node);

int XmlReadInt(const XmlNode& node, const std::wstring_view name);
std::wstring XmlReadStr(const XmlNode& node, const std::wstring_view name);

void XmlWriteInt(XmlNode& node, const std::wstring_view name, const int value);
void XmlWriteStr(XmlNode& node, const std::wstring_view name,
                 const std::wstring_view value,
                 pugi::xml_node_type node_type = pugi::node_pcdata);

void XmlWriteChildNodes(
    XmlNode& parent_node,
    const std::vector<std::wstring>& input,
    const std::wstring_view name,
    pugi::xml_node_type node_type = pugi::node_pcdata);

pugi::xml_parse_result XmlLoadFileToDocument(
    XmlDocument& document,
    const std::wstring_view path,
    const unsigned int options = pugi::parse_default,
    const pugi::xml_encoding encoding = pugi::xml_encoding::encoding_auto);
bool XmlSaveDocumentToFile(
    const XmlDocument& document,
    const std::wstring_view path,
    const std::wstring_view indent = L"\t",
    const unsigned int flags = pugi::format_default,
    const pugi::xml_encoding encoding = pugi::xml_encoding::encoding_utf8);

std::wstring XmlReadMetaVersion(const XmlDocument& document);
void XmlWriteMetaVersion(XmlDocument& document, const std::wstring_view version);
