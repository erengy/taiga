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

#include "base/types.h"
#include "library/anime_episode.h"
#include "track/feed_filter.h"

namespace pugi {
class xml_document;
}

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

  TorrentCategory GetTorrentCategory() const;

  bool operator<(const FeedItem& item) const;
  bool operator==(const FeedItem& item) const;

  std::wstring info_link;
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

  std::wstring GetDataPath();
  bool Load();
  bool Load(const std::wstring& data);

  FeedCategory category;

private:
  void Load(const pugi::xml_document& document);
};

////////////////////////////////////////////////////////////////////////////////

class Aggregator {
public:
  Aggregator();
  ~Aggregator() {}

  Feed* GetFeed(FeedCategory category);

  bool CheckFeed(FeedCategory category, const std::wstring& source, bool automatic = false);
  bool Download(FeedCategory category, const FeedItem* feed_item);

  void HandleFeedCheck(Feed& feed, const std::string& data, bool automatic);
  void HandleFeedDownload(Feed& feed, const std::string& data);
  bool ValidateFeedDownload(const HttpRequest& http_request, HttpResponse& http_response);

  void ExamineData(Feed& feed);
  void ParseDescription(FeedItem& feed_item, const std::wstring& source);

  bool LoadArchive();
  bool SaveArchive() const;
  void AddToArchive(const std::wstring& file);
  bool SearchArchive(const std::wstring& file) const;

  FeedFilterManager filter_manager;

private:
  bool CompareFeedItems(const GenericFeedItem& item1, const GenericFeedItem& item2);
  FeedItem* FindFeedItemByLink(Feed& feed, const std::wstring& link);
  void HandleFeedDownloadOpen(FeedItem& feed_item, const std::wstring& file);

  std::vector<std::wstring> download_queue_;
  std::vector<Feed> feeds_;
  std::vector<std::wstring> file_archive_;
};

extern class Aggregator Aggregator;

#endif  // TAIGA_TRACK_FEED_H