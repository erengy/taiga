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

namespace track {

enum class FeedSource {
  Unknown,
  Acgnx,
  AniDex,
  AnimeBytes,
  AnimeTosho,
  Minglong,
  NyaaPantsu,
  NyaaSi,
  SubsPlease,
  TokyoToshokan,
};

class FeedItem;

FeedSource GetFeedSource(const std::wstring& channel_link);
void ParseFeedItemFromSource(const FeedSource source, FeedItem& feed_item);

}  // namespace track
