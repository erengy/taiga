/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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

// =============================================================================

class CEventItem {
public:
  int Episode, ID, Index, Mode, Score, Status;
  wstring Reason, Tags, Time;
};

class CEventList {
public:
  CEventList();
  virtual ~CEventList() {}
  
  void Add(int index, int id, int episode, int score, int status, wstring tags, wstring time, int mode);
  void Check();
  void Clear();
  int  GetLastWatchedEpisode(int index);
  void Remove(unsigned int index);
  
  unsigned int Index;
  wstring User;
  vector<CEventItem> Item;
};

class CEventQueue {
public:
  CEventQueue();
  virtual ~CEventQueue() {}

  void Add(wstring user, int index, int id, int episode, int score, int status, wstring tags, wstring time, int mode);
  void Check();
  void Clear();
  int  GetItemCount();
  int  GetLastWatchedEpisode(int index);
  int  GetUserIndex(wstring user = L"");
  bool IsEmpty();
  void Remove(int index = -1);
  void Show();

  bool UpdateInProgress;
  vector<CEventList> List;
};

extern CEventQueue EventQueue;

#endif // EVENT_H