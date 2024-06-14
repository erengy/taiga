/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <optional>
#include <queue>
#include <string>
#include <vector>

#include "base/time.h"
#include "base/xml.h"
#include "media/anime.h"
#include "track/episode.h"

namespace library {

enum class QueueSearch {
  DateStart,
  DateEnd,
  Episode,
  Notes,
  RewatchedTimes,
  Rewatching,
  Score,
  Status,
};

enum class QueueItemMode {
  Add,
  Delete,
  Update,
};

struct QueueItem {
  bool enabled = true;
  int anime_id = 0;
  QueueItemMode mode = QueueItemMode::Update;
  std::wstring time;

  std::optional<int> episode;
  std::optional<anime::MyStatus> status;
  std::optional<int> score;
  std::optional<Date> date_start;
  std::optional<Date> date_finish;
  std::optional<bool> enable_rewatching;
  std::optional<int> rewatched_times;
  std::optional<std::wstring> notes;
};

class Queue {
public:
  void Add(QueueItem& item, bool save = true);
  void Check(bool automatic = true);
  void Clear(bool save = true);
  void Merge(bool save = true);
  bool IsQueued(int anime_id) const;
  QueueItem* FindItem(int anime_id, QueueSearch search_mode);
  QueueItem* GetCurrentItem();
  int GetItemCount();
  void Remove(int index = 0, bool save = true, bool refresh = true, bool to_history = true);
  void RemoveDisabled(bool save = true, bool refresh = true);

  std::vector<QueueItem> items;
  bool updating = false;
};

class ConfirmationQueue {
public:
  void Add(const anime::Episode& episode);
  void Process();

private:
  bool in_process_ = false;
  std::queue<anime::Episode> queue_;
};

QueueItemMode TranslateQueueItemModeFromString(const std::wstring& mode);
std::wstring TranslateQueueItemModeToString(const QueueItemMode mode);

inline Queue queue;
inline ConfirmationQueue confirmation_queue;

}  // namespace library
