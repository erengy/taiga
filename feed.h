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

enum FeedItemState {
  FEEDITEM_DISCARDED = -1,
  FEEDITEM_BLANK = 0,
  FEEDITEM_SELECTED = 1
};

enum ItemDiscardType {
  DISCARDTYPE_NORMAL = 0,
  DISCARDTYPE_GRAYOUT,
  DISCARDTYPE_HIDE
};

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
  FeedItem() : state(FEEDITEM_BLANK), discard_type(DISCARDTYPE_NORMAL) {}
  virtual ~FeedItem() {};

  int index;
  wstring magnet_link;

  FeedItemState state;
  ItemDiscardType discard_type;

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
  FEED_FILTER_ELEMENT_META_ID,
  FEED_FILTER_ELEMENT_META_STATUS,
  FEED_FILTER_ELEMENT_META_TYPE,
  FEED_FILTER_ELEMENT_META_EPISODES,
  FEED_FILTER_ELEMENT_META_DATE_START,
  FEED_FILTER_ELEMENT_META_DATE_END,
  FEED_FILTER_ELEMENT_USER_STATUS,
  FEED_FILTER_ELEMENT_LOCAL_EPISODE_AVAILABLE,
  FEED_FILTER_ELEMENT_EPISODE_TITLE,
  FEED_FILTER_ELEMENT_EPISODE_NUMBER,
  FEED_FILTER_ELEMENT_EPISODE_VERSION,
  FEED_FILTER_ELEMENT_EPISODE_GROUP,
  FEED_FILTER_ELEMENT_EPISODE_VIDEO_RESOLUTION,
  FEED_FILTER_ELEMENT_EPISODE_VIDEO_TYPE,
  FEED_FILTER_ELEMENT_FILE_TITLE,
  FEED_FILTER_ELEMENT_FILE_CATEGORY,
  FEED_FILTER_ELEMENT_FILE_DESCRIPTION,
  FEED_FILTER_ELEMENT_FILE_LINK,
  FEED_FILTER_ELEMENT_COUNT
};

enum FeedFilterOperator {
  FEED_FILTER_OPERATOR_EQUALS,
  FEED_FILTER_OPERATOR_NOTEQUALS,
  FEED_FILTER_OPERATOR_ISGREATERTHAN,
  FEED_FILTER_OPERATOR_ISGREATERTHANOREQUALTO,
  FEED_FILTER_OPERATOR_ISLESSTHAN,
  FEED_FILTER_OPERATOR_ISLESSTHANOREQUALTO,
  FEED_FILTER_OPERATOR_BEGINSWITH,
  FEED_FILTER_OPERATOR_ENDSWITH,
  FEED_FILTER_OPERATOR_CONTAINS,
  FEED_FILTER_OPERATOR_NOTCONTAINS,
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
  FEED_FILTER_ACTION_DISCARD_GRAYOUT,
  FEED_FILTER_ACTION_DISCARD_HIDE,
  FEED_FILTER_ACTION_SELECT,
  FEED_FILTER_ACTION_PREFER
};

class FeedFilter {
public:
  FeedFilter() : action(0), enabled(true), match(FEED_FILTER_MATCH_ALL) {}
  virtual ~FeedFilter() {}
  FeedFilter& operator=(const FeedFilter& filter);

  void AddCondition(int element, int op, const wstring& value);
  void Filter(Feed& feed, FeedItem& item, bool recursive);
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

enum FeedFilterShortcodeType {
  FEED_FILTER_SHORTCODE_ACTION,
  FEED_FILTER_SHORTCODE_ELEMENT,
  FEED_FILTER_SHORTCODE_MATCH,
  FEED_FILTER_SHORTCODE_OPERATOR,
};

class FeedFilterManager {
public:
  FeedFilterManager();

  void InitializePresets();
  void InitializeShortcodes();

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

  wstring GetShortcodeFromIndex(FeedFilterShortcodeType type, int index);
  int GetIndexFromShortcode(FeedFilterShortcodeType type, const wstring& shortcode);

public:
  vector<FeedFilter> filters;
  vector<FeedFilterPreset> presets;

private:
  std::map<int, wstring> action_shortcodes_;
  std::map<int, wstring> element_shortcodes_;
  std::map<int, wstring> match_shortcodes_;
  std::map<int, wstring> operator_shortcodes_;
};

// =============================================================================

class Aggregator {
public:
  Aggregator();
  virtual ~Aggregator() {}

  Feed* Get(int category);

  bool Notify(const Feed& feed);
  void ParseDescription(FeedItem& feed_item, const wstring& source);

  bool LoadArchive();
  bool SaveArchive();
  bool SearchArchive(const wstring& file);

private:
  bool CompareFeedItems(const GenericFeedItem& item1, const GenericFeedItem& item2);

public:
  vector<Feed> feeds;
  vector<wstring> file_archive;
  FeedFilterManager filter_manager;
};

extern Aggregator Aggregator;

#endif // FEED_H