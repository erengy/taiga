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

#include "media/library/queue.h"

#include "base/log.h"
#include "base/string.h"
#include "base/xml.h"
#include "media/library/history.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "sync/sync.h"
#include "taiga/announce.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/version.h"
#include "track/media.h"
#include "track/scanner.h"
#include "ui/ui.h"

void HistoryQueue::Add(QueueItem& item, bool save) {
  auto anime = anime::db.Find(item.anime_id);

  // Add to user list
  if (anime && !anime->IsInList())
    if (item.mode != taiga::kHttpServiceDeleteLibraryEntry)
      anime->AddtoUserList();

  // Validate values
  if (anime && anime->IsInList()) {
    if (item.episode)
      if (anime->GetMyLastWatchedEpisode() == *item.episode || *item.episode < 0)
        item.episode.reset();
    if (item.score)
      if (anime->GetMyScore() == *item.score || *item.score < 0 || *item.score > anime::kUserScoreMax)
        item.score.reset();
    if (item.status)
      if (anime->GetMyStatus() == *item.status || *item.status < anime::kMyStatusFirst || *item.status >= anime::kMyStatusLast)
        item.status.reset();
    if (item.enable_rewatching)
      if (anime->GetMyRewatching() == *item.enable_rewatching)
        item.enable_rewatching.reset();
    if (item.rewatched_times)
      if (anime->GetMyRewatchedTimes() == *item.rewatched_times)
        item.rewatched_times.reset();
    if (item.tags)
      if (anime->GetMyTags() == *item.tags)
        item.tags.reset();
    if (item.notes)
      if (anime->GetMyNotes() == *item.notes)
        item.notes.reset();
    if (item.date_start)
      if (anime->GetMyDateStart() == *item.date_start)
        item.date_start.reset();
    if (item.date_finish)
      if (anime->GetMyDateEnd() == *item.date_finish)
        item.date_finish.reset();
  }
  switch (item.mode) {
    case taiga::kHttpServiceUpdateLibraryEntry:
      if (!item.episode &&
          !item.score &&
          !item.status &&
          !item.enable_rewatching &&
          !item.rewatched_times &&
          !item.tags &&
          !item.notes &&
          !item.date_start &&
          !item.date_finish)
        return;
      break;
  }

  // Edit previous item with the same ID...
  bool add_new_item = true;
  if (!History.queue.updating) {
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
      if (it->anime_id == item.anime_id && it->enabled) {
        if (it->mode != taiga::kHttpServiceAddLibraryEntry &&
            it->mode != taiga::kHttpServiceDeleteLibraryEntry) {
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
            if (item.tags)
              it->tags = *item.tags;
            if (item.notes)
              it->notes = *item.notes;
            if (item.date_start)
              it->date_start = *item.date_start;
            if (item.date_finish)
              it->date_finish = *item.date_finish;
            add_new_item = false;
          }
          if (!add_new_item) {
            it->mode = taiga::kHttpServiceUpdateLibraryEntry;
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

  if (anime && save) {
    // Save
    history->Save();

    // Announce
    if (item.episode) {
      anime::Episode episode;
      episode.anime_id = anime->GetId();
      episode.set_episode_number(*item.episode);
      MediaPlayers.play_status = track::recognition::PlayStatus::Updated;
      Announcer.Do(taiga::kAnnounceToHttp | taiga::kAnnounceToTwitter, &episode);
    }

    // Check new episode
    if (item.episode) {
      anime->SetNextEpisodePath(L"");
      ScanAvailableEpisodesQuick(anime->GetId());
    }

    ui::OnHistoryAddItem(item);

    // Update
    Check(true);
  }
}

void HistoryQueue::Check(bool automatic) {
  if (items.empty())
    return;

  if (!items[index].enabled) {
    LOGD(L"Item is disabled, removing...");
    Remove(index, true, true, false);
    Check(automatic);
    return;
  }

  auto anime_item = anime::db.Find(items[index].anime_id);
  if (!anime_item) {
    LOGW(L"Item not found in list, removing... ID: {}", items[index].anime_id);
    Remove(index, true, true, false);
    Check(automatic);
    return;
  }

  if (automatic && !Settings.GetBool(taiga::kApp_Option_EnableSync)) {
    items[index].reason = L"Automatic synchronization is disabled";
    LOGD(items[index].reason);
    return;
  }

  if (!sync::UserAuthenticated()) {
    sync::AuthenticateUser();
    return;
  }

  History.queue.updating = true;
  ui::ChangeStatusText(L"Updating list... (" + anime::GetPreferredTitle(*anime_item) + L")");

  sync::UpdateLibraryEntry(items[index]);
}

void HistoryQueue::Clear(bool save) {
  items.clear();
  index = 0;

  ui::OnHistoryChange();

  if (save)
    history->Save();
}

void HistoryQueue::Merge(bool save) {
  while (auto queue_item = GetCurrentItem()) {
    anime::db.UpdateItem(*queue_item);
    History.queue.Remove(index, false, true, true);
  }

  ui::OnHistoryChange();

  if (save) {
    history->Save();
    anime::db.SaveList();
  }
}

bool HistoryQueue::IsQueued(int anime_id) const {
  for (const auto& item : items) {
    if (item.anime_id == anime_id)
      return true;
  }
  return false;
}

QueueItem* HistoryQueue::FindItem(int anime_id, QueueSearch search_mode) {
  for (auto it = items.rbegin(); it != items.rend(); ++it) {
    if (it->anime_id == anime_id && it->enabled) {
      switch (search_mode) {
        // Date
        case QueueSearch::DateStart:
          if (it->date_start)
            return &(*it);
          break;
        case QueueSearch::DateEnd:
          if (it->date_finish)
            return &(*it);
          break;
        // Episode
        case QueueSearch::Episode:
          if (it->episode)
            return &(*it);
          break;
        // Notes
        case QueueSearch::Notes:
          if (it->notes)
            return &(*it);
          break;
        // Rewatched times
        case QueueSearch::RewatchedTimes:
          if (it->rewatched_times)
            return &(*it);
          break;
        // Rewatching
        case QueueSearch::Rewatching:
          if (it->enable_rewatching)
            return &(*it);
          break;
        // Score
        case QueueSearch::Score:
          if (it->score)
            return &(*it);
          break;
        // Status
        case QueueSearch::Status:
          if (it->status)
            return &(*it);
          break;
        // Tags
        case QueueSearch::Tags:
          if (it->tags)
            return &(*it);
          break;
        // Default
        default:
          return &(*it);
      }
    }
  }
  return nullptr;
}

QueueItem* HistoryQueue::GetCurrentItem() {
  if (!items.empty())
    return &items.at(index);

  return nullptr;
}

int HistoryQueue::GetItemCount() {
  int count = 0;

  for (auto it = items.begin(); it != items.end(); ++it)
    if (it->enabled)
      count++;

  return count;
}

void HistoryQueue::Remove(int index, bool save, bool refresh, bool to_history) {
  if (index == -1)
    index = this->index;

  if (index < static_cast<int>(items.size())) {
    auto it = items.begin() + index;
    const QueueItem queue_item = *it;

    if (to_history && queue_item.episode && *queue_item.episode > 0) {
      HistoryItem history_item;
      history_item.anime_id = queue_item.anime_id;
      history_item.episode = *queue_item.episode;
      history_item.time = queue_item.time;
      history->items.push_back(history_item);
      if (history->limit > 0 &&
          static_cast<int>(history->items.size()) > history->limit) {
        history->items.erase(history->items.begin());
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
    history->Save();
}

void HistoryQueue::RemoveDisabled(bool save, bool refresh) {
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
    history->Save();
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
