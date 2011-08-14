/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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
#include "animelist.h"
#include "http.h"

// =============================================================================

class GenericFeedItem {
public:
  GenericFeedItem() : IsPermaLink(true) {}
  virtual ~GenericFeedItem() {}

  wstring Title, Link, Description,
    Author, Category, Comments, Enclosure, GUID, PubDate, Source;
  bool IsPermaLink;
};

class FeedItem : public GenericFeedItem {
public:
  FeedItem() : Download(true) {}
  virtual ~FeedItem() {};

  int Index;
  bool Download;
  
  class CEpisodeData : public Episode {
  public:
    CEpisodeData() : NewEpisode(false) {}
    virtual ~CEpisodeData() {}
    wstring FileSize;
    bool NewEpisode;
  } EpisodeData;
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
  wstring Title, Link, Description;

  // Optional channel elements
  // (see www.rssboard.org/rss-specification for more information)
  /*
  wstring language, copyright, managingEditor, webMaster, pubDate, 
    lastBuildDate, category, generator, docs, cloud, ttl, image, 
    rating, textInput, skipHours, skipDays;
  */
  
  // Feed items
  vector<FeedItem> Items;
};

class Feed : public GenericFeed {
public:
  Feed();
  virtual ~Feed();

  bool Check(const wstring& source);
  bool Download(int index);
  int ExamineData();
  wstring GetDataPath();
  HICON GetIcon();
  bool Read();

  int Category, DownloadIndex, Ticker;
  wstring Title, Link;
  class HttpClient Client;

private:
  HICON m_hIcon;
};

// =============================================================================

enum FeedFilterElement {
  FEED_FILTER_ELEMENT_TITLE,
  FEED_FILTER_ELEMENT_CATEGORY,
  FEED_FILTER_ELEMENT_DESCRIPTION,
  FEED_FILTER_ELEMENT_LINK,
  FEED_FILTER_ELEMENT_ANIME_ID,
  FEED_FILTER_ELEMENT_ANIME_TITLE,
  FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS,
  FEED_FILTER_ELEMENT_ANIME_MYSTATUS,
  FEED_FILTER_ELEMENT_ANIME_EPISODE_NUMBER,
  FEED_FILTER_ELEMENT_ANIME_EPISODE_VERSION,
  FEED_FILTER_ELEMENT_ANIME_EPISODE_AVAILABLE,
  FEED_FILTER_ELEMENT_ANIME_GROUP,
  FEED_FILTER_ELEMENT_ANIME_VIDEO_RESOLUTION,
  FEED_FILTER_ELEMENT_ANIME_VIDEO_TYPE,
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
  FeedFilterCondition() : Element(0), Operator(0) {}
  virtual ~FeedFilterCondition() {}
  FeedFilterCondition& operator=(const FeedFilterCondition& condition);

  void Reset();

  int Element;
  int Operator;
  wstring Value;
};

enum FeedFilterAction {
  FEED_FILTER_ACTION_DISCARD,
  FEED_FILTER_ACTION_SELECT,
  FEED_FILTER_ACTION_PREFER
};

class FeedFilter {
public:
  FeedFilter() : Action(0), Enabled(true), Match(FEED_FILTER_MATCH_ALL) {}
  virtual ~FeedFilter() {}
  FeedFilter& operator=(const FeedFilter& filter);

  void AddCondition(int element, int op, const wstring& value);
  bool Filter(Feed& feed, FeedItem& item, bool recursive);
  void Reset();

  wstring Name;
  bool Enabled;
  int Action, Match;
  vector<int> AnimeIds;
  vector<FeedFilterCondition> Conditions;
};

class FeedFilterPreset {
public:
  FeedFilterPreset() : Default(false) {}
  bool Default;
  wstring Description;
  FeedFilter Filter;
};

class FeedFilterManager {
public:
  FeedFilterManager();

  void AddPresets();
  void AddFilter(int action, int match = FEED_FILTER_MATCH_ALL, 
    bool enabled = true, const wstring& name = L"");
  void Cleanup();
  int Filter(Feed& feed);

  wstring CreateNameFromConditions(const FeedFilter& filter);
  wstring TranslateCondition(const FeedFilterCondition& condition);
  wstring TranslateConditions(const FeedFilter& filter, size_t index);
  wstring TranslateElement(int element);
  wstring TranslateOperator(int op);
  wstring TranslateValue(const FeedFilterCondition& condition);
  wstring TranslateMatching(int match);
  wstring TranslateAction(int action);
  
  vector<FeedFilter> Filters;
  vector<FeedFilterPreset> Presets;
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

  vector<Feed> Feeds;
  vector<wstring> FileArchive;
  FeedFilterManager FilterManager;

private:
  bool CompareFeedItems(const GenericFeedItem& item1, const GenericFeedItem& item2);
};

extern Aggregator Aggregator;

#endif // FEED_H