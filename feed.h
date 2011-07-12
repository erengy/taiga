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

class CGenericFeedItem {
public:
  CGenericFeedItem() : IsPermaLink(true) {}
  virtual ~CGenericFeedItem() {}

  wstring Title, Link, Description,
    Author, Category, Comments, Enclosure, GUID, PubDate, Source;
  bool IsPermaLink;
};

class CFeedItem : public CGenericFeedItem {
public:
  CFeedItem() : Download(true) {}
  virtual ~CFeedItem() {};

  int Index;
  bool Download;
  
  class CEpisodeData : public CEpisode {
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

class CGenericFeed {
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
  vector<CFeedItem> Item;
};

class CFeed : public CGenericFeed {
public:
  CFeed() : Category(0), DownloadIndex(-1), Ticker(0), m_hIcon(NULL) {}
  virtual ~CFeed() {
    if (m_hIcon) DeleteObject(m_hIcon);
  }

  bool Check(const wstring& source);
  bool Download(int index);
  int ExamineData();
  wstring GetDataPath();
  HICON GetIcon();
  bool Read();

  int Category, DownloadIndex, Ticker;
  wstring Title, Link;
  CHTTPClient Client;

private:
  HICON m_hIcon;
};

// =============================================================================

enum FeedFilterElement {
  FEED_FILTER_ELEMENT_TITLE,
  FEED_FILTER_ELEMENT_CATEGORY,
  FEED_FILTER_ELEMENT_LINK,
  FEED_FILTER_ELEMENT_DESCRIPTION,
  FEED_FILTER_ELEMENT_ANIME_ID,
  FEED_FILTER_ELEMENT_ANIME_TITLE,
  FEED_FILTER_ELEMENT_ANIME_GROUP,
  FEED_FILTER_ELEMENT_ANIME_EPISODE,
  FEED_FILTER_ELEMENT_ANIME_EPISODEAVAILABLE,
  FEED_FILTER_ELEMENT_ANIME_RESOLUTION,
  FEED_FILTER_ELEMENT_ANIME_MYSTATUS,
  FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS,
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

class CFeedFilterCondition {
public:
  CFeedFilterCondition() : Element(0), Operator(0) {}
  virtual ~CFeedFilterCondition() {}

  int Element;
  int Operator;
  wstring Value;
};

enum FeedFilterAction {
  FEED_FILTER_ACTION_EXCLUDE,
  FEED_FILTER_ACTION_INCLUDE,
  FEED_FILTER_ACTION_PREFER
};

class CFeedFilter {
public:
  CFeedFilter() : Action(0), Enabled(true), Match(FEED_FILTER_MATCH_ALL) {}
  virtual ~CFeedFilter() {}
  CFeedFilter& operator=(const CFeedFilter& filter);

  void AddCondition(int element, int op, const wstring& value);
  bool Filter(CFeed& feed, CFeedItem& item, bool recursive);

  wstring Name;
  bool Enabled;
  vector<CFeedFilterCondition> Conditions;
  int Action, Match;
};

class CFeedFilterManager {
public:
  void AddDefaultFilters();
  void AddFilter(int action, int match = FEED_FILTER_MATCH_ALL, 
    bool enabled = true, const wstring& name = L"");
  int Filter(CFeed& feed);

  wstring TranslateCondition(const CFeedFilterCondition& condition);
  wstring TranslateConditions(const CFeedFilter& filter, size_t index);
  wstring TranslateElement(int element);
  wstring TranslateOperator(int op);
  wstring TranslateValue(const CFeedFilterCondition& condition);
  
  vector<CFeedFilter> Filters;
};

// =============================================================================

class CAggregator {
public:
  CAggregator();
  virtual ~CAggregator() {}

  CFeed* Get(int category);

  bool Notify(const CFeed& feed);
  bool SearchArchive(const wstring& file);
  void ParseDescription(CFeedItem& feed_item, const wstring& source);

  vector<CFeed> Feeds;
  vector<wstring> FileArchive;
  CFeedFilterManager FilterManager;

private:
  bool CompareFeedItems(const CGenericFeedItem& item1, const CGenericFeedItem& item2);
};

extern CAggregator Aggregator;

#endif // FEED_H