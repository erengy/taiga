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

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "base/rss.h"
#include "track/episode.h"
#include "track/feed_source.h"

namespace pugi {
class xml_document;
}

namespace track {

enum class FeedItemState {
  Blank,
  DiscardedNormal,
  DiscardedInactive,
  DiscardedHidden,
  Selected,
};

enum class TorrentCategory {
  Anime,
  Batch,
  Other,
};

class FeedItem : public rss::Item {
public:
  void Discard(int option);
  bool IsDiscarded() const;

  bool operator<(const FeedItem& item) const;
  bool operator==(const FeedItem& item) const;

  std::wstring info_link;
  std::wstring magnet_link;
  FeedItemState state = FeedItemState::Blank;
  TorrentCategory torrent_category = TorrentCategory::Anime;
  std::optional<size_t> seeders;
  std::optional<size_t> leechers;
  std::optional<size_t> downloads;
  uint64_t file_size = 0;

  class EpisodeData : public anime::Episode {
  public:
    bool new_episode = false;
  } episode_data;
};

class Feed {
public:
  std::wstring GetDataPath() const;

  bool Load();
  bool Load(const std::wstring& data);

  FeedSource source = FeedSource::Unknown;
  rss::Channel channel;
  std::vector<FeedItem> items;

private:
  void Load(const pugi::xml_document& document);
};

TorrentCategory GetTorrentCategory(const FeedItem& item);
std::wstring TranslateTorrentCategory(TorrentCategory category);
TorrentCategory TranslateTorrentCategory(const std::wstring& str);

}  // namespace track
