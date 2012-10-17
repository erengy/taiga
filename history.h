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

#ifndef HISTORY_H
#define HISTORY_H

#include "std.h"
#include "myanimelist.h"

// =============================================================================

enum EventSearchMode {
  EVENT_SEARCH_DATE_START = 1,
  EVENT_SEARCH_DATE_END,
  EVENT_SEARCH_EPISODE,
  EVENT_SEARCH_REWATCH,
  EVENT_SEARCH_SCORE,
  EVENT_SEARCH_STATUS,
  EVENT_SEARCH_TAGS
};

class EventItem : public mal::AnimeValues {
public:
  EventItem();
  virtual ~EventItem() {}
  
public:
  bool enabled;
  int anime_id, mode;
  wstring reason, time;
};

class EventList {
public:
  EventList();
  virtual ~EventList() {}
  
  void Add(EventItem& item);
  void Check();
  void Clear();
  EventItem* FindItem(int anime_id, int search_mode = 0);
  void Remove(unsigned int index, bool refresh = true);
  void RemoveDisabled(bool refresh = true);
  
public:
  unsigned int index;
  vector<EventItem> items;
  wstring user;
};

class EventQueue {
public:
  EventQueue();
  virtual ~EventQueue() {}

  void Add(EventItem& item, bool save = true, wstring user = L"");
  void Check();
  void Clear(bool save = true);
  
  EventItem* FindItem(int anime_id, int search_mode = 0);
  EventList* FindList(wstring user = L"");
  
  int GetItemCount();
  bool IsEmpty();
  void Remove(int index = -1, bool save = true, bool refresh = true);
  void RemoveDisabled(bool save = true, bool refresh = true);

public:
  vector<EventList> list;
  bool updating;
};

class History {
public:
  EventQueue queue;
};

extern class History History;

#endif // HISTORY_H