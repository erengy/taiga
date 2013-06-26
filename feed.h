/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#ifndef FEED_H
#define FEED_H

#include "std.h"
#include "anime_episode.h"
#include "http.h"

// =============================================================================

class GenericFeedItem {
public:
  GenericFeedItem() : is_permalink(true) {}
  virtual ~GenericFeedItem() {}

  wstring title, link, description,
    author, category, comments, enclosure, guid, pub_date, source;
  bool is_permalink;
};

class FeedItem : public GenericFeedItem {
public:
  FeedItem() : download(true) {}
  virtual ~FeedItem() {};

  bool download;
  int index;
  wstring magnet_link;
  
  class EpisodeData : public anime::Episode {
  public:
    EpisodeData() : new_episode(false) {}
    wstring file_size;
    bool new_episode;
  } episode_data;
};

// =============================================================================

enum FeedCategory {
  // Broadcatching for torrent files and DDL
  FEED_CATEGORY_LINK,
  // News around the web
  FEED_CATEGORY_TEXT,
  // Airing times for anime titles
  FEED_CATEGORY_TIME
};

enum TorrentCategory {
  TORRENT_ANIME,
  TORRENT_BATCH,
  TORRENT_OTHER
};

class GenericFeed {
public:
  // Required channel elements
  wstring title, link, description;

  // Optional channel elements
  // (see www.rssboard.org/rss-specification for more information)
  /*
  wstring language, copyright, managingEditor, webMaster, pubDate, 
    lastBuildDate, category, generator, docs, cloud, ttl, image, 
    rating, textInput, skipHours, skipDays;
  */
  
  // Feed items
  vector<FeedItem> items;
};

class Feed : public GenericFeed {
public:
  Feed();
  virtual ~Feed();

  bool Check(const wstring& source, bool automatic = false);
  bool Download(int index);
  bool ExamineData();
  wstring GetDataPath();
  HICON GetIcon();
  bool Load();

public:
  int category, download_index, ticker;
  wstring title, link;
  HttpClient client;

private:
  HICON icon_;
};

// =============================================================================

enum FeedFilterElement {
  FEED_FILTER_ELEMENT_TITLE,
  FEED_FILTER_ELEMENT_CATEGORY,
  FEED_FILTER_ELEMENT_DESCRIPTION,
  FEED_FILTER_ELEMENT_LINK,
  FEED_FILTER_ELEMENT_ANIME_ID,
  FEED_FILTER_ELEMENT_ANIME_TITLE,
  FEED_FILTER_ELEMENT_ANIME_SERIES_STATUS,
  FEED_FILTER_ELEMENT_ANIME_MY_STATUS,
  FEED_FILTER_ELEMENT_ANIME_EPISODE_NUMBER,
  FEED_FILTER_ELEMENT_ANIME_EPISODE_VERSION,
  FEED_FILTER_ELEMENT_ANIME_EPISODE_AVAILABLE,
  FEED_FILTER_ELEMENT_ANIME_GROUP,
  FEED_FILTER_ELEMENT_ANIME_VIDEO_RESOLUTION,
  FEED_FILTER_ELEMENT_ANIME_VIDEO_TYPE,
  FEED_FILTER_ELEMENT_ANIME_SERIES_DATE_START,
  FEED_FILTER_ELEMENT_ANIME_SERIES_DATE_END,
  FEED_FILTER_ELEMENT_ANIME_SERIES_EPISODES,
  FEED_FILTER_ELEMENT_ANIME_SERIES_TYPE,
  FEED_FILTER_ELEMENT_COUNT
};

enum FeedFilterOperator {
  FEED_FILTER_OPERATOR_IS,
  FEED_FILTER_OPERATOR_ISNOT,
  FEED_FILTER_OPERATOR_ISGREATERTHAN,
  FEED_FILTER_OPERATOR_ISLESSTHAN,
  FEED_FILTER_OPERATOR_BEGINSWITH,
  FEED_FILTER_OPERATOR_ENDSWITH,
  FEED_FILTER_OPERATOR_CONTAINS,
  FEED_FILTER_OPERATOR_CONTAINSNOT,
  FEED_FILTER_OPERATOR_COUNT
};

enum FeedFilterMatch {
  FEED_FILTER_MATCH_ALL,
  FEED_FILTER_MATCH_ANY
};

class FeedFilterCondition {
public:
  FeedFilterCondition() : element(0), op(0) {}
  virtual ~FeedFilterCondition() {}
  FeedFilterCondition& operator=(const FeedFilterCondition& condition);

  void Reset();

public:
  int element;
  int op;
  wstring value;
};

enum FeedFilterAction {
  FEED_FILTER_ACTION_DISCARD,
  FEED_FILTER_ACTION_SELECT,
  FEED_FILTER_ACTION_PREFER
};

class FeedFilter {
public:
  FeedFilter() : action(0), enabled(true), match(FEED_FILTER_MATCH_ALL) {}
  virtual ~FeedFilter() {}
  FeedFilter& operator=(const FeedFilter& filter);

  void AddCondition(int element, int op, const wstring& value);
  bool Filter(Feed& feed, FeedItem& item, bool recursive);
  void Reset();

public:
  wstring name;
  bool enabled;
  int action, match;
  vector<int> anime_ids;
  vector<FeedFilterCondition> conditions;
};

class FeedFilterPreset {
public:
  FeedFilterPreset() : is_default(false) {}
  wstring description;
  FeedFilter filter;
  bool is_default;
};

class FeedFilterManager {
public:
  FeedFilterManager();

  void AddPresets();
  void AddFilter(int action, int match = FEED_FILTER_MATCH_ALL, 
    bool enabled = true, const wstring& name = L"");
  void Cleanup();
  void Filter(Feed& feed, bool preferences);
  void FilterArchived(Feed& feed);
  bool IsItemDownloadAvailable(Feed& feed);
  void MarkNewEpisodes(Feed& feed);

  wstring CreateNameFromConditions(const FeedFilter& filter);
  wstring TranslateCondition(const FeedFilterCondition& condition);
  wstring TranslateConditions(const FeedFilter& filter, size_t index);
  wstring TranslateElement(int element);
  wstring TranslateOperator(int op);
  wstring TranslateValue(const FeedFilterCondition& condition);
  wstring TranslateMatching(int match);
  wstring TranslateAction(int action);
  
public:
  vector<FeedFilter> filters;
  vector<FeedFilterPreset> presets;
};

// =============================================================================

class Aggregator {
public:
  Aggregator();
  virtual ~Aggregator() {}

  Feed* Get(int category);

  bool Notify(const Feed& feed);
  bool SearchArchive(const wstring& file);
  void ParseDescription(FeedItem& feed_item, const wstring& source);

private:
  bool CompareFeedItems(const GenericFeedItem& item1, const GenericFeedItem& item2);

public:
  vector<Feed> feeds;
  vector<wstring> file_archive;
  FeedFilterManager filter_manager;
};

extern Aggregator Aggregator;

#endif // FEED_H