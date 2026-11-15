/**
 * Taiga
 * Copyright (C) 2010-2026, Eren Okka
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

namespace rss {

// Reference: https://www.rssboard.org/rss-specification

struct Channel {
  std::string title;
  std::string link;
  std::string description;
  // Optional elements are not implemented.
};

struct Item {
  struct Category {
    std::string domain;
    std::string value;
  };

  struct Enclosure {
    std::string url;
    std::string length;  // in bytes
    std::string type;
  };

  struct Guid {
    bool isPermalink = true;
    std::string value;
  };

  struct Source {
    std::string url;
    std::string name;
  };

  std::string title;        // The title of the item.
  std::string link;         // The URL of the item.
  std::string description;  // The item synopsis.
  std::string author;       // Email address of the author of the item.
  Category category;        // Includes the item in one or more categories.
  std::string comments;     // URL of a page for comments relating to the item.
  Enclosure enclosure;      // Describes a media object that is attached to the item.
  Guid guid;                // A string that uniquely identifies the item.
  std::string pubDate;      // Indicates when the item was published.
  Source source;            // The RSS channel that the item came from.

  std::unordered_map<std::string, std::string> namespaceElements;
};

struct Feed {
  Channel channel;
  std::vector<Item> items;
};

}  // namespace rss
