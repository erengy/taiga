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

#include "media/library/queue.h"

#include "base/log.h"
#include "base/string.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "media/library/history.h"
#include "sync/sync.h"
#include "taiga/announce.h"
#include "taiga/settings.h"
#include "track/media.h"
#include "track/scanner.h"
#include "ui/ui.h"

namespace library {

static void ValidateQueueItem(QueueItem& item, const anime::Item& anime_item) {
  if (item.episode)
    if (anime_item.GetMyLastWatchedEpisode() == *item.episode || *item.episode < 0)
      item.episode.reset();
  if (item.score)
    if (anime_item.GetMyScore() == *item.score || *item.score < 0 || *item.score > anime::kUserScoreMax)
      item.score.reset();
  if (item.status)
    if (anime_item.GetMyStatus() == *item.status || *item.status == anime::MyStatus::NotInList)
      item.status.reset();
  if (item.enable_rewatching)
    if (anime_item.GetMyRewatching() == *item.enable_rewatching)
      item.enable_rewatching.reset();
  if (item.rewatched_times)
    if (anime_item.GetMyRewatchedTimes() == *item.rewatched_times)
      item.rewatched_times.reset();
  if (item.notes)
    if (anime_item.GetMyNotes() == *item.notes)
      item.notes.reset();
  if (item.date_start)
    if (anime_item.GetMyDateStart() == *item.date_start)
      item.date_start.reset();
  if (item.date_finish)
    if (anime_item.GetMyDateEnd() == *item.date_finish)
      item.date_finish.reset();
}

void Queue::Add(QueueItem& item, bool save) {
  const auto anime_item = anime::db.Find(item.anime_id);

  // Add to user list
  if (anime_item && !anime_item->IsInList())
    if (item.mode != QueueItemMode::Delete)
      anime_item->AddtoUserList();

  // Validate values
  if (anime_item && anime_item->IsInList()) {
    ValidateQueueItem(item, *anime_item);
  }
  switch (item.mode) {
    case QueueItemMode::Update:
      if (!item.episode &&
          !item.score &&
          !item.status &&
          !item.enable_rewatching &&
          !item.rewatched_times &&
          !item.notes &&
          !item.date_start &&
          !item.date_finish)
        return;
      break;
  }

  // Edit previous item with the same ID...
  bool add_new_item = true;
  if (!updating) {
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
      if (it->anime_id == item.anime_id && it->enabled) {
        if (it->mode != QueueItemMode::Add &&
            it->mode != QueueItemMode::Delete) {
          if (!item.episode || (!it->episode && it == items.rbegin())) {
            if (item.episode)
              it->episode = *item.episode;
            if (item.score)
              it->score = *item.score;
            if (item.status)
              it->status = *item.status;
            if (item.enable_rewatching)
              it->enable_rewatching = *item.enable_rewatching;
            if (item.rewatched_times)
              it->rewatched_times = *item.rewatched_times;
            if (item.notes)
              it->notes = *item.notes;
            if (item.date_start)
              it->date_start = *item.date_start;
            if (item.date_finish)
              it->date_finish = *item.date_finish;
            add_new_item = false;
          }
          if (!add_new_item) {
            it->mode = QueueItemMode::Update;
            it->time = GetDate().to_string() + L" " + GetTime();
          }
          break;
        }
      }
    }
  }
  // ...or add a new one
  if (add_new_item) {
    if (item.time.empty())
      item.time = GetDate().to_string() + L" " + GetTime();
    items.push_back(item);
  }

  if (anime_item && save) {
    // Save
    history.Save();

    // Announce
    if (item.episode) {
      anime::Episode episode;
      episode.anime_id = anime_item->GetId();
      episode.set_episode_number(*item.episode);
      track::media_players.play_status = track::recognition::PlayStatus::Updated;
      taiga::announcer.Do(taiga::kAnnounceToHttp, &episode);
    }

    // Check new episode
    if (item.episode) {
      anime_item->SetNextEpisodePath(L"");
      ScanAvailableEpisodesQuick(anime_item->GetId());
    }

    ui::OnHistoryAddItem(item);

    // Update
    Check(true);
  }
}

void Queue::Check(bool automatic) {
  if (updating || items.empty())
    return;

  const auto& queue_item = items.front();

  if (!queue_item.enabled) {
    LOGD(L"Item is disabled, removing...");
    Remove(0, true, true, false);
    Check(automatic);
    return;
  }

  const auto anime_item = anime::db.Find(queue_item.anime_id);
  if (!anime_item) {
    LOGW(L"Item not found in list, removing... ID: {}", queue_item.anime_id);
    Remove(0, true, true, false);
    Check(automatic);
    return;
  }

  if (automatic && !taiga::settings.GetAppOptionEnableSync()) {
    LOGD(L"Automatic synchronization is disabled");
    return;
  }

  if (!sync::IsUserAuthenticated()) {
    sync::AuthenticateUser();
    return;
  }

  updating = true;

  switch (queue_item.mode) {
    case QueueItemMode::Add:
      sync::AddLibraryEntry(queue_item);
      break;
    case QueueItemMode::Delete:
      sync::DeleteLibraryEntry(queue_item.anime_id);
      break;
    case QueueItemMode::Update:
      sync::UpdateLibraryEntry(queue_item);
      break;
  }
}

void Queue::Clear(bool save) {
  items.clear();

  ui::OnHistoryChange();

  if (save)
    history.Save();
}

void Queue::Merge(bool save) {
  while (auto queue_item = GetCurrentItem()) {
    anime::db.UpdateItem(*queue_item);
    Remove(0, false, true, true);
  }

  ui::OnHistoryChange();

  if (save) {
    history.Save();
    anime::db.SaveList();
  }
}

bool Queue::IsQueued(int anime_id) const {
  for (const auto& item : items) {
    if (item.anime_id == anime_id)
      return true;
  }
  return false;
}

QueueItem* Queue::FindItem(int anime_id, QueueSearch search_mode) {
  const auto has_value = [&search_mode](const QueueItem& item) {
    switch (search_mode) {
      case QueueSearch::DateStart:
        return item.date_start.has_value();
      case QueueSearch::DateEnd:
        return item.date_finish.has_value();
      case QueueSearch::Episode:
        return item.episode.has_value();
      case QueueSearch::Notes:
        return item.notes.has_value();
      case QueueSearch::RewatchedTimes:
        return item.rewatched_times.has_value();
      case QueueSearch::Rewatching:
        return item.enable_rewatching.has_value();
      case QueueSearch::Score:
        return item.score.has_value();
      case QueueSearch::Status:
        return item.status.has_value();
      default:
        return false;
    }
  };

  for (auto it = items.rbegin(); it != items.rend(); ++it) {
    auto& item = *it; 
    if (item.anime_id == anime_id && item.enabled && has_value(item)) {
      return &item;
    }
  }

  return nullptr;
}

QueueItem* Queue::GetCurrentItem() {
  if (!items.empty())
    return &items.front();

  return nullptr;
}

int Queue::GetItemCount() {
  int count = 0;

  for (auto it = items.begin(); it != items.end(); ++it) {
    if (it->enabled)
      ++count;
  }

  return count;
}

void Queue::Remove(int index, bool save, bool refresh, bool to_history) {
  if (0 <= index && index < static_cast<int>(items.size())) {
    auto it = items.begin() + index;
    const QueueItem queue_item = *it;

    if (to_history && queue_item.episode && *queue_item.episode > 0) {
      HistoryItem history_item;
      history_item.anime_id = queue_item.anime_id;
      history_item.episode = *queue_item.episode;
      history_item.time = queue_item.time;
      history.items.push_back(history_item);
      if (history.limit > 0 &&
          static_cast<int>(history.items.size()) > history.limit) {
        history.items.erase(history.items.begin());
      }
    }

    if (queue_item.episode) {
      auto anime_item = anime::db.Find(queue_item.anime_id);
      if (anime_item &&
          anime_item->GetMyLastWatchedEpisode() == *queue_item.episode) {
        // Next episode path is no longer valid
        anime_item->SetNextEpisodePath(L"");
      }
    }

    items.erase(it);

    if (refresh)
      ui::OnHistoryChange(&queue_item);
  }

  if (save)
    history.Save();
}

void Queue::RemoveDisabled(bool save, bool refresh) {
  bool needs_refresh = false;

  for (size_t i = 0; i < items.size(); i++) {
    if (!items.at(i).enabled) {
      items.erase(items.begin() + i);
      needs_refresh = true;
      i--;
    }
  }

  if (refresh && needs_refresh)
    ui::OnHistoryChange();

  if (save)
    history.Save();
}

////////////////////////////////////////////////////////////////////////////////

void ConfirmationQueue::Add(const anime::Episode& episode) {
  queue_.push(episode);
}

void ConfirmationQueue::Process() {
  if (in_process_)
    return;
  in_process_ = true;

  while (!queue_.empty()) {
    anime::Episode& episode = queue_.front();
    int choice = ui::OnHistoryProcessConfirmationQueue(episode);
    if (choice != IDNO) {
      bool change_status = (choice == IDCANCEL);
      auto anime_item = anime::db.Find(episode.anime_id);
      if (anime_item)
        AddToQueue(*anime_item, episode, change_status);
    }
    queue_.pop();
  }

  in_process_ = false;
}

////////////////////////////////////////////////////////////////////////////////

QueueItemMode TranslateQueueItemModeFromString(const std::wstring& mode) {
  if (mode == L"add") {
    return QueueItemMode::Add;
  } else if (mode == L"delete") {
    return QueueItemMode::Delete;
  } else {
    return QueueItemMode::Update;
  }
}

std::wstring TranslateQueueItemModeToString(const QueueItemMode mode) {
  switch (mode) {
    case QueueItemMode::Add:
      return L"add";
    case QueueItemMode::Delete:
      return L"delete";
    default:
    case QueueItemMode::Update:
      return L"update";
  }
}

}  // namespace library
