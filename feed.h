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

// =============================================================================

class CFeedItem {
public:
  CFeedItem();
  ~CFeedItem() {};

  wstring Title, Link, Description;
  wstring Author, Category, Comments, Enclosure, GUID, PubDate, Source;

  int Index;
  wstring Size;
  CEpisode EpisodeData;
  BOOL NewItem, Download;
};

class CFeed {
public:
  // Required channel elements
  wstring Title, Link, Description;

  // Optional elements
  /*
  wstring language, copyright, managingEditor, webMaster, pubDate, 
    lastBuildDate, category, generator, docs, cloud, ttl, image, 
    rating, textInput, skipHours, skipDays;
  */
  
  // Feed items
  vector<CFeedItem> Item;
};

// =============================================================================

enum FeedFilterOption {
  FEED_FILTER_EXCLUDE,
  FEED_FILTER_INCLUDE,
  FEED_FILTER_PREFERENCE
};

enum FeedFilterType {
  FEED_FILTER_KEYWORD,
  FEED_FILTER_AIRINGSTATUS,
  FEED_FILTER_WATCHINGSTATUS
};

class CFeedFilter {
public:
  int Option, Type;
  wstring Value;
};

// =============================================================================

enum TorrentCategory {
  TORRENT_ANIME,
  TORRENT_BATCH,
  TORRENT_OTHER
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
  
  CFeed Feed;
  vector<wstring> Archive;

private:
  void ParseDescription(CFeedItem& feed_item, const wstring& source);
};

extern CTorrents Torrents;

#endif // FEED_H