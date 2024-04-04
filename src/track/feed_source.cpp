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

#include <map>

#include "track/feed_source.h"

#include "base/file.h"
#include "base/string.h"
#include "base/url.h"
#include "track/feed.h"

namespace track {

FeedSource GetFeedSource(const std::wstring& channel_link) {
  static const std::map<std::wstring, FeedSource> sources{
    {L"acgnx", FeedSource::Acgnx},
    {L"anidex", FeedSource::AniDex},
    {L"animebytes", FeedSource::AnimeBytes},
    {L"animetosho", FeedSource::AnimeTosho},
    {L"minglong", FeedSource::Minglong},
    {L"nyaa.net", FeedSource::NyaaPantsu},
    {L"nyaa.pantsu", FeedSource::NyaaPantsu},
    {L"nyaa.pt", FeedSource::NyaaPantsu},
    {L"nyaa.si", FeedSource::NyaaSi},
    {L"subsplease", FeedSource::SubsPlease},
    {L"tokyotosho", FeedSource::TokyoToshokan},
  };

  const Url url(channel_link);
  for (const auto& pair : sources) {
    if (InStr(url.host(), pair.first, 0, true) > -1) {
      return pair.second;
    }
  }

  return FeedSource::Unknown;
}

void ParseFeedItemFromSource(const FeedSource source, FeedItem& feed_item) {
  const auto parse_magnet_link = [&feed_item]() {
    if (StartsWith(feed_item.enclosure.url, L"magnet:")) {
      feed_item.magnet_link = feed_item.enclosure.url;
    } else {
      feed_item.magnet_link =
          InStr(feed_item.description, L"<a href=\"magnet:?", L"\">");
      if (!feed_item.magnet_link.empty())
        feed_item.magnet_link = L"magnet:?" + feed_item.magnet_link;
    }
  };

  switch (source) {
    case FeedSource::Acgnx: {
      std::vector<std::wstring> parts;
      Split(feed_item.description, L" | ", parts);
      if (parts.size() > 2) {
        feed_item.file_size = ParseSizeString(parts.at(2));
      }
      parse_magnet_link();
      break;
    }

    case FeedSource::AniDex:
      feed_item.file_size = ParseSizeString(
          InStr(feed_item.description, L"Size: ", L" |"));
      if (InStr(feed_item.description, L" Batch ") > -1)
        feed_item.torrent_category = TorrentCategory::Batch;
      parse_magnet_link();
      break;

    case FeedSource::AnimeTosho:
      if (InStr(feed_item.link, L"/view/") > -1 &&
          !feed_item.enclosure.url.empty()) {
        feed_item.link = feed_item.enclosure.url;
      }
      feed_item.info_link = feed_item.guid.value;
      feed_item.file_size = ParseSizeString(
          InStr(feed_item.description, L"<strong>Total Size</strong>: ", L"<"));
      parse_magnet_link();
      break;

    case FeedSource::Minglong:
      feed_item.file_size = ParseSizeString(
          InStr(feed_item.description, L"Size: ", L"<"));
      break;

    case FeedSource::NyaaPantsu:
      feed_item.info_link = feed_item.guid.value;
      feed_item.file_size = ToUint64(feed_item.enclosure.length);
      break;

    case FeedSource::NyaaSi: {
      feed_item.info_link = feed_item.guid.value;
      auto& elements = feed_item.namespace_elements;
      if (elements.count(L"nyaa:size"))
        feed_item.file_size = ParseSizeString(elements[L"nyaa:size"]);
      if (elements.count(L"nyaa:seeders"))
        feed_item.seeders = ToInt(elements[L"nyaa:seeders"]);
      if (elements.count(L"nyaa:leechers"))
        feed_item.leechers = ToInt(elements[L"nyaa:leechers"]);
      if (elements.count(L"nyaa:downloads"))
        feed_item.downloads = ToInt(elements[L"nyaa:downloads"]);
      break;
    }

    case FeedSource::SubsPlease: {
      auto& elements = feed_item.namespace_elements;
      if (elements.count(L"subsplease:size"))
        feed_item.file_size = ParseSizeString(elements[L"subsplease:size"]);
      break;
    }

    case FeedSource::TokyoToshokan:
      feed_item.file_size = ParseSizeString(
          InStr(feed_item.description, L"Size: ", L"<"));
      parse_magnet_link();
      feed_item.description = InStr(feed_item.description, L"Comment: ", L"");
      feed_item.info_link = feed_item.guid.value;
      break;
  }
}

}  // namespace track
