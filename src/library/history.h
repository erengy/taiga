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

#ifndef TAIGA_LIBRARY_HISTORY_H
#define TAIGA_LIBRARY_HISTORY_H

#include <string>
#include <queue>
#include <vector>

#include "base/optional.h"
#include "base/time.h"
#include "library/anime_episode.h"

enum QueueSearchMode {
  kQueueSearchDateStart = 1,
  kQueueSearchDateEnd,
  kQueueSearchEpisode,
  kQueueSearchRewatching,
  kQueueSearchScore,
  kQueueSearchStatus,
  kQueueSearchTags
};

class AnimeValues {
public:
  Optional<int> episode;
  Optional<int> status;
  Optional<int> score;
  Optional<Date> date_start;
  Optional<Date> date_finish;
  Optional<int> enable_rewatching;
  Optional<wstring> tags;
};

class HistoryItem : public AnimeValues {
public:
  HistoryItem();
  virtual ~HistoryItem() {}

  bool enabled;
  int anime_id;
  int mode;
  std::wstring reason;
  std::wstring time;
};

class History;

class HistoryQueue {
public:
  HistoryQueue();
  ~HistoryQueue() {}

  void Add(HistoryItem& item, bool save = true);
  void Check(bool automatic = true);
  void Clear(bool save = true);
  HistoryItem* FindItem(int anime_id, int search_mode = 0);
  HistoryItem* GetCurrentItem();
  int GetItemCount();
  void Remove(int index = -1, bool save = true, bool refresh = true, bool to_history = true);
  void RemoveDisabled(bool save = true, bool refresh = true);

  size_t index;
  vector<HistoryItem> items;
  History* history;
  bool updating;
};

class History {
public:
  History();
  ~History() {}

  bool Load();
  bool Save();

  std::vector<HistoryItem> items;
  HistoryQueue queue;
  int limit;
};

class ConfirmationQueue {
public:
  ConfirmationQueue();
  virtual ~ConfirmationQueue() {}

  void Add(const anime::Episode& episode);
  void Process();

private:
  bool in_process_;
  std::queue<anime::Episode> queue_;
};

extern class History History;
extern class ConfirmationQueue ConfirmationQueue;

#endif  // TAIGA_LIBRARY_HISTORY_H