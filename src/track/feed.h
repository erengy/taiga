/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#ifndef TAIGA_TRACK_FEED_H
#define TAIGA_TRACK_FEED_H

#include <string>
#include <vector>

#include "library/anime_episode.h"
#include "track/feed_filter.h"

enum FeedItemState {
  kFeedItemBlank,
  kFeedItemDiscardedNormal,
  kFeedItemDiscardedInactive,
  kFeedItemDiscardedHidden,
  kFeedItemSelected
};

enum FeedCategory {
  // Broadcatching for torrent files and DDL
  kFeedCategoryLink,
  // News around the web
  kFeedCategoryText,
  // Airing times for anime titles
  kFeedCategoryTime
};

enum TorrentCategory {
  kTorrentCategoryAnime,
  kTorrentCategoryBatch,
  kTorrentCategoryOther
};

////////////////////////////////////////////////////////////////////////////////

class GenericFeedItem {
public:
  GenericFeedItem();
  virtual ~GenericFeedItem() {}

  std::wstring title,
               link,
               description,
               author,
               category,
               comments,
               enclosure,
               guid,
               pub_date,
               source;

  bool permalink;
};

class FeedItem : public GenericFeedItem {
public:
  FeedItem();
  ~FeedItem() {};

  void Discard(int option);
  bool IsDiscarded() const;

  bool operator<(const FeedItem& item) const;

  int index;
  std::wstring magnet_link;
  FeedItemState state;

  class EpisodeData : public anime::Episode {
  public:
    EpisodeData() : new_episode(false) {}
    std::wstring file_size;
    bool new_episode;
  } episode_data;
};

////////////////////////////////////////////////////////////////////////////////

class GenericFeed {
public:
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
  Feed();
  ~Feed() {}

  bool Check(const std::wstring& source, bool automatic = false);
  bool Download(int index);
  bool ExamineData();
  std::wstring GetDataPath();
  bool Load();

  FeedCategory category;
  int download_index;
};

////////////////////////////////////////////////////////////////////////////////

class Aggregator {
public:
  Aggregator();
  ~Aggregator() {}

  Feed* Get(FeedCategory category);

  bool Notify(const Feed& feed);
  void ParseDescription(FeedItem& feed_item, const std::wstring& source);

  bool LoadArchive();
  bool SaveArchive();
  bool SearchArchive(const std::wstring& file);

  std::vector<Feed> feeds;
  std::vector<std::wstring> file_archive;
  FeedFilterManager filter_manager;

private:
  bool CompareFeedItems(const GenericFeedItem& item1, const GenericFeedItem& item2);
};

extern Aggregator Aggregator;

#endif  // TAIGA_TRACK_FEED_H