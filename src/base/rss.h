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
#include <unordered_map>
#include <vector>

namespace pugi {
class xml_document;
class xml_node;
}

namespace rss {

// Reference: http://www.rssboard.org/rss-specification

struct Channel {
  std::wstring title;
  std::wstring link;
  std::wstring description;
  // Optional elements are not implemented.
};

struct Item {
  struct Category {
    std::wstring domain;
    std::wstring value;
  };

  struct Enclosure {
    std::wstring url;
    std::wstring length;  // in bytes
    std::wstring type;
  };

  struct Guid {
    bool is_permalink = true;
    std::wstring value;
  };

  struct Source {
    std::wstring url;
    std::wstring name;
  };

  std::wstring title;        // The title of the item.
  std::wstring link;         // The URL of the item.
  std::wstring description;  // The item synopsis.
  std::wstring author;       // Email address of the author of the item.
  Category category;         // Includes the item in one or more categories.
  std::wstring comments;     // URL of a page for comments relating to the item.
  Enclosure enclosure;       // Describes a media object that is attached to the item.
  Guid guid;                 // A string that uniquely identifies the item.
  std::wstring pub_date;     // Indicates when the item was published.
  Source source;             // The RSS channel that the item came from.

  std::unordered_map<std::wstring, std::wstring> namespace_elements;
};

struct Feed {
  Channel channel;
  std::vector<Item> items;
};

Channel ParseChannel(const pugi::xml_node& node);
Item ParseItem(const pugi::xml_node& node);
Feed ParseDocument(const pugi::xml_document& document);

}  // namespace rss
