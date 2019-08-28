/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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

#pragma once

#include <string>
#include <optional>
#include <queue>
#include <vector>

#include "base/time.h"
#include "base/xml.h"
#include "library/anime_episode.h"

enum class QueueSearch {
  DateStart,
  DateEnd,
  Episode,
  Notes,
  RewatchedTimes,
  Rewatching,
  Score,
  Status,
  Tags,
};

class AnimeValues {
public:
  std::optional<int> episode;
  std::optional<int> status;
  std::optional<int> score;
  std::optional<Date> date_start;
  std::optional<Date> date_finish;
  std::optional<bool> enable_rewatching;
  std::optional<int> rewatched_times;
  std::optional<std::wstring> tags;
  std::optional<std::wstring> notes;
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
  void Merge(bool save = true);
  bool IsQueued(int anime_id) const;
  HistoryItem* FindItem(int anime_id, QueueSearch search_mode);
  HistoryItem* GetCurrentItem();
  int GetItemCount();
  void Remove(int index = -1, bool save = true, bool refresh = true, bool to_history = true);
  void RemoveDisabled(bool save = true, bool refresh = true);

  size_t index;
  std::vector<HistoryItem> items;
  History* history;
  bool updating;
};

class History {
public:
  History();
  ~History() {}

  void Clear(bool save = true);
  bool Load();
  bool Save();

  void HandleCompatibility(const std::wstring& meta_version);

  std::vector<HistoryItem> items;
  HistoryQueue queue;
  int limit;

private:
  void ReadQueue(const pugi::xml_document& document);
  void ReadQueueInCompatibilityMode(const pugi::xml_document& document);

  int TranslateModeFromString(const std::wstring& mode);
  std::wstring TranslateModeToString(int mode);
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
