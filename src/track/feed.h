/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "base/types.h"
#include "library/anime_episode.h"
#include "track/feed_filter.h"

namespace pugi {
class xml_document;
}

enum class FeedItemState {
  Blank,
  DiscardedNormal,
  DiscardedInactive,
  DiscardedHidden,
  Selected,
};

enum class FeedSource {
  Unknown,
  AniDex,
  AnimeBytes,
  Minglong,
  NyaaPantsu,
  NyaaSi,
  TokyoToshokan,
};

enum class TorrentCategory {
  Anime,
  Batch,
  Other,
};

////////////////////////////////////////////////////////////////////////////////

struct GenericFeedItem {
  std::wstring title,
               link,
               description,
               author,
               category,
               comments,
               enclosure_url,
               enclosure_length,
               enclosure_type,
               guid,
               pub_date,
               source;

  bool permalink = true;
};

class FeedItem : public GenericFeedItem {
public:
  void Discard(int option);
  bool IsDiscarded() const;

  bool operator<(const FeedItem& item) const;
  bool operator==(const FeedItem& item) const;

  std::map<std::wstring, std::wstring> elements;
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
    EpisodeData() : new_episode(false) {}
    bool new_episode;
  } episode_data;
};

TorrentCategory GetTorrentCategory(const FeedItem& item);
std::wstring TranslateTorrentCategory(TorrentCategory category);
TorrentCategory TranslateTorrentCategory(const std::wstring& str);

////////////////////////////////////////////////////////////////////////////////

struct GenericFeed {
  // Required channel elements
  std::wstring title,
               link,
               description;

  // Optional channel elements are not implemented.
  // See http://www.rssboard.org/rss-specification for more information.

  // Feed items
  std::vector<FeedItem> items;
};

class Feed : public GenericFeed {
public:
  std::wstring GetDataPath();
  bool Load();
  bool Load(const std::wstring& data);

  FeedSource source = FeedSource::Unknown;

private:
  void Load(const pugi::xml_document& document);
};

////////////////////////////////////////////////////////////////////////////////

class Aggregator {
public:
  Feed& GetFeed();

  bool CheckFeed(const std::wstring& source, bool automatic = false);
  bool Download(const FeedItem* feed_item);

  void HandleFeedCheck(Feed& feed, const std::string& data, bool automatic);
  void HandleFeedDownload(Feed& feed, const std::string& data);
  void HandleFeedDownloadError(Feed& feed);
  bool ValidateFeedDownload(const HttpRequest& http_request, HttpResponse& http_response);

  void FindFeedSource(Feed& feed) const;
  void ExamineData(Feed& feed);
  void ParseFeedItem(FeedSource source, FeedItem& feed_item);
  void CleanupDescription(std::wstring& description);

  size_t GetArchiveSize() const;
  bool LoadArchive();
  bool SaveArchive() const;
  void AddToArchive(const std::wstring& file);
  void ClearArchive();
  bool SearchArchive(const std::wstring& file) const;

  FeedFilterManager filter_manager;

private:
  FeedItem* FindFeedItemByLink(Feed& feed, const std::wstring& link);
  void HandleFeedDownloadOpen(FeedItem& feed_item, const std::wstring& file);
  bool IsMagnetLink(const FeedItem& feed_item) const;

  std::vector<std::wstring> download_queue_;
  Feed feed_;
  std::vector<std::wstring> file_archive_;
};

extern class Aggregator Aggregator;
