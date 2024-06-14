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

#include "base/rss.h"

#include "base/html.h"
#include "base/string.h"
#include "base/xml.h"

namespace rss {

Channel ParseChannel(const XmlNode& node) {
  Channel channel;

  channel.title = node.child_value(L"title");
  channel.link = node.child_value(L"link");
  channel.description = node.child_value(L"description");

  return channel;
}

Item ParseItem(const XmlNode& node) {
  Item item;

  item.title = node.child_value(L"title");
  item.link = node.child_value(L"link");
  item.description = node.child_value(L"description");
  item.author = node.child_value(L"author");
  item.comments = node.child_value(L"comments");
  item.pub_date = node.child_value(L"pubDate");

  if (const auto category_node = node.child(L"category")) {
    item.category.domain = category_node.attribute(L"domain").as_string();
    item.category.value = category_node.child_value();
  }

  if (const auto enclosure_node = node.child(L"enclosure")) {
    item.enclosure.url = enclosure_node.attribute(L"url").as_string();
    item.enclosure.length = enclosure_node.attribute(L"length").as_string();
    item.enclosure.type = enclosure_node.attribute(L"type").as_string();
  }

  if (const auto guid_node = node.child(L"guid")) {
    item.guid.is_permalink = guid_node.attribute(L"isPermaLink").as_bool(true);
    item.guid.value = guid_node.child_value();
  }

  for (const auto& element : node.children()) {
    if (InStr(element.name(), L":") > -1) {
      item.namespace_elements[element.name()] = element.child_value();
    }
  }

  return item;
}

Feed ParseDocument(const XmlDocument& document) {
  Feed feed;

  const auto channel_node = document.child(L"rss").child(L"channel");

  feed.channel = ParseChannel(channel_node);

  for (auto node : channel_node.children(L"item")) {
    auto item = ParseItem(node);

    // All elements of an item are optional, however at least one of title or
    // description must be present.
    if (item.title.empty() && item.description.empty())
      continue;

    DecodeHtmlEntities(item.title);
    DecodeHtmlEntities(item.description);

    feed.items.push_back(item);
  }

  return feed;
}

}  // namespace rss
