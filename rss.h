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

#ifndef RSS_H
#define RSS_H

#include "std.h"
#include "animelist.h"

// =============================================================================

enum TorrentCategory {
  TORRENT_ANIME,
  TORRENT_BATCH,
  TORRENT_OTHER
};

class CRSSFeedItem {
public:
  CRSSFeedItem();
  ~CRSSFeedItem() {};

  wstring Category, Description, Link, Size, Title;
  BOOL NewItem, Download;
  int Index;
  CEpisode EpisodeData;
};

class CRSSFeed {
public:
  wstring Title, Link, Description;
  vector<CRSSFeedItem> Item;
};

enum RssFilterOption {
  RSS_FILTER_EXCLUDE,
  RSS_FILTER_INCLUDE,
  RSS_FILTER_PREFERENCE
};

enum RssFilterType {
  RSS_FILTER_KEYWORD,
  RSS_FILTER_AIRINGSTATUS,
  RSS_FILTER_WATCHINGSTATUS
};

class CRSSFilter {
public:
  int Option, Type;
  wstring Value;
};

class CTorrents {
public:
  BOOL Check(const wstring& source);
  BOOL Compare();
  BOOL Download(int index);
  BOOL Read();
  BOOL SearchArchive(const wstring& file);
  BOOL Tip();

  void AddFilter(int option, int type, wstring value);
  BOOL Filter(int feed_index, int anime_index);
  
  CRSSFeed Feed;
  vector<wstring> Archive;

private:
  void ParseDescription(CRSSFeedItem& feed_item, const wstring& source);
};

extern CTorrents Torrents;

#endif // RSS_H