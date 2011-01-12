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

#ifndef EVENT_H
#define EVENT_H

#include "std.h"
#include "myanimelist.h"

// =============================================================================

enum EventSearchMode {
  EVENT_SEARCH_EPISODE = 1,
  EVENT_SEARCH_REWATCH,
  EVENT_SEARCH_SCORE,
  EVENT_SEARCH_STATUS,
  EVENT_SEARCH_TAGS
};

class CEventItem : public CMALAnimeValues {
public:
  CEventItem() : Index(0), AnimeID(0), AnimeIndex(0), Mode(0) {}
  int Index;
  int AnimeID, AnimeIndex, Mode;
  wstring Reason, Time;
};

class CEventList {
public:
  CEventList();
  virtual ~CEventList() {}
  
  void Add(CEventItem& item);
  void Check();
  void Clear();
  void Remove(unsigned int index);
  CEventItem* SearchItem(int anime_index, int search_mode = 0);
  
  unsigned int Index;
  wstring User;
  vector<CEventItem> Item;
};

class CEventQueue {
public:
  CEventQueue();
  virtual ~CEventQueue() {}

  void Add(CEventItem& item, wstring user = L"");
  void Check();
  void Clear();
  int GetItemCount();
  int GetUserIndex(wstring user = L"");
  bool IsEmpty();
  void Remove(int index = -1);
  CEventItem* SearchItem(int anime_index, int search_mode = 0);
  void Show();

  bool UpdateInProgress;
  vector<CEventList> List;
};

extern CEventQueue EventQueue;

#endif // EVENT_H