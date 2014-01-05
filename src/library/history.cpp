/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#include "base/foreach.h"
#include "base/logger.h"
#include "base/string.h"
#include "base/xml.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "library/history.h"
#include "sync/sync.h"
#include "taiga/announce.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "ui/ui.h"

class ConfirmationQueue ConfirmationQueue;
class History History;

HistoryItem::HistoryItem()
    : anime_id(anime::ID_UNKNOWN),
      enabled(true),
      mode(0) {
}

HistoryQueue::HistoryQueue()
    : index(0),
      history(nullptr),
      updating(false) {
}

void HistoryQueue::Add(HistoryItem& item, bool save) {
  auto anime = AnimeDatabase.FindItem(item.anime_id);

  // Add to user list
  if (anime && !anime->IsInList())
    if (item.mode != taiga::kHttpServiceDeleteLibraryEntry)
      anime->AddtoUserList();

  // Validate values
  if (anime && anime->IsInList()) {
    if (item.episode)
      if (anime->GetMyLastWatchedEpisode() == *item.episode || *item.episode < 0)
        item.episode.Reset();
    if (item.score)
      if (anime->GetMyScore() == *item.score || *item.score < 0 || *item.score > 10)
        item.score.Reset();
    if (item.status)
      if (anime->GetMyStatus() == *item.status || *item.status < 1 || *item.status == 5 || *item.status > 6)
        item.status.Reset();
    if (item.enable_rewatching)
      if (anime->GetMyRewatching() == *item.enable_rewatching)
        item.enable_rewatching.Reset();
    if (item.tags)
      if (anime->GetMyTags() == *item.tags)
        item.tags.Reset();
    if (item.date_start)
      if (anime->GetMyDateStart() == *item.date_start)
        item.date_start.Reset();
    if (item.date_finish)
      if (anime->GetMyDateEnd() == *item.date_finish)
        item.date_finish.Reset();
  }
  switch (item.mode) {
    case taiga::kHttpServiceUpdateLibraryEntry:
      if (!item.episode &&
          !item.score &&
          !item.status &&
          !item.enable_rewatching &&
          !item.tags &&
          !item.date_start &&
          !item.date_finish)
        return;
      break;
  }

  // Edit previous item with the same ID...
  bool add_new_item = true;
  if (!History.queue.updating) {
    foreach_r_(it, items) {
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
            if (item.tags)
              it->tags = *item.tags;
            if (item.date_start)
              it->date_start = *item.date_start;
            if (item.date_finish)
              it->date_finish = *item.date_finish;
            add_new_item = false;
          }
          if (!add_new_item) {
            it->mode = taiga::kHttpServiceUpdateLibraryEntry;
            it->time = (wstring)GetDate() + L" " + GetTime();
          }
          break;
        }
      }
    }
  }
  // ...or add a new one
  if (add_new_item) {
    if (item.time.empty()) item.time = (wstring)GetDate() + L" " + GetTime();
    items.push_back(item);
  }

  if (anime && save) {
    // Save
    history->Save();

    // Announce
    if (Taiga.logged_in && item.episode) {
      anime::Episode episode;
      episode.anime_id = anime->GetId();
      episode.number = ToWstr(*item.episode);
      Taiga.play_status = taiga::kPlayStatusUpdated;
      Announcer.Do(taiga::kAnnounceToHttp | taiga::kAnnounceToTwitter, &episode);
    }

    // Check new episode
    if (item.episode) {
      anime->SetNewEpisodePath(L"");
      anime::CheckEpisodes(*anime, 0);
    }

    ui::OnHistoryAddItem(item);

    // Update
    Check();
  }
}

void HistoryQueue::Check(bool automatic) {
  // Check
  if (items.empty()) {
    return;
  }
  if (!items[index].enabled) {
    LOG(LevelDebug, L"Item is disabled, removing...");
    Remove(index);
    Check();
    return;
  }
  if (!Taiga.logged_in) {
    items[index].reason = L"Not logged in";
    return;
  }
  if (automatic && !Settings.GetBool(taiga::kApp_Option_EnableSync)) {
    items[index].reason = L"Synchronization is disabled";
    return;
  }

  // Compare ID with anime list
  auto anime_item = AnimeDatabase.FindItem(items[index].anime_id);
  if (!anime_item) {
    LOG(LevelWarning, L"Item not found in list, removing... ID: " +
                      ToWstr(items[index].anime_id));
    Remove(index);
    Check();
    return;
  }

  // Update
  History.queue.updating = true;
  ui::ChangeStatusText(L"Updating list...");
  AnimeValues* anime_values = static_cast<AnimeValues*>(&items[index]);
  sync::UpdateLibraryEntry(*anime_values, items[index].anime_id,
      static_cast<taiga::HttpClientMode>(items[index].mode));
}

void HistoryQueue::Clear(bool save) {
  items.clear();
  index = 0;

  ui::OnHistoryChange();

  if (save)
    history->Save();
}

HistoryItem* HistoryQueue::FindItem(int anime_id, int search_mode) {
  for (auto it = items.rbegin(); it != items.rend(); ++it) {
    if (it->anime_id == anime_id && it->enabled) {
      switch (search_mode) {
        // Date
        case kQueueSearchDateStart:
          if (it->date_start)
            return &(*it);
          break;
        case kQueueSearchDateEnd:
          if (it->date_finish)
            return &(*it);
          break;
        // Episode
        case kQueueSearchEpisode:
          if (it->episode)
            return &(*it);
          break;
        // Re-watching
        case kQueueSearchRewatching:
          if (it->enable_rewatching)
            return &(*it);
          break;
        // Score
        case kQueueSearchScore:
          if (it->score)
            return &(*it);
          break;
        // Status
        case kQueueSearchStatus:
          if (it->status)
            return &(*it);
          break;
        // Tags
        case kQueueSearchTags:
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

HistoryItem* HistoryQueue::GetCurrentItem() {
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
    auto history_item = items.begin() + index;
    
    if (to_history && history_item->episode) {
      history->items.push_back(*history_item);
      if (history->limit > 0 &&
          static_cast<int>(history->items.size()) > history->limit) {
        history->items.erase(history->items.begin());
      }
    }

    items.erase(history_item);

    if (refresh)
      ui::OnHistoryChange();
  }

  if (save) history->Save();
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

History::History()
    : limit(0) {  // Limit of history items (0 for unlimited)
  queue.history = this;
}

void History::Clear(bool save) {
  items.clear();

  ui::OnHistoryChange();

  if (save)
    Save();
}

bool History::Load() {
  items.clear();
  queue.items.clear();

  xml_document document;
  wstring path = taiga::GetPath(taiga::kPathUserHistory);
  xml_parse_result parse_result = document.load_file(path.c_str());

  if (parse_result.status != pugi::status_ok)
    return false;

  // Items
  xml_node node_items = document.child(L"history").child(L"items");
  foreach_xmlnode_(item, node_items, L"item") {
    HistoryItem history_item;
    history_item.anime_id = item.attribute(L"anime_id").as_int(anime::ID_NOTINLIST);
    history_item.episode = item.attribute(L"episode").as_int();
    history_item.time = item.attribute(L"time").value();
    items.push_back(history_item);
  }
  // Queue events
  xml_node node_queue = document.child(L"history").child(L"queue");
  foreach_xmlnode_(item, node_queue, L"item") {
    HistoryItem history_item;
    history_item.anime_id = item.attribute(L"anime_id").as_int(anime::ID_NOTINLIST);
    history_item.mode = item.attribute(L"mode").as_int();
    history_item.time = item.attribute(L"time").value();
    #define READ_ATTRIBUTE_INT(x, y) \
        if (!item.attribute(y).empty()) x = item.attribute(y).as_int();
    #define READ_ATTRIBUTE_STR(x, y) \
        if (!item.attribute(y).empty()) x = item.attribute(y).as_string();
    #define READ_ATTRIBUTE_DATE(x, y) \
        if (!item.attribute(y).empty()) x = (Date)item.attribute(y).as_string();
    READ_ATTRIBUTE_INT(history_item.episode, L"episode");
    READ_ATTRIBUTE_INT(history_item.score, L"score");
    READ_ATTRIBUTE_INT(history_item.status, L"status");
    READ_ATTRIBUTE_INT(history_item.enable_rewatching, L"enable_rewatching");
    READ_ATTRIBUTE_STR(history_item.tags, L"tags");
    READ_ATTRIBUTE_DATE(history_item.date_start, L"date_start");
    READ_ATTRIBUTE_DATE(history_item.date_finish, L"date_finish");
    #undef READ_ATTRIBUTE_DATE
    #undef READ_ATTRIBUTE_STR
    #undef READ_ATTRIBUTE_INT
    queue.Add(history_item, false);
  }

  return true;
}

bool History::Save() {
  xml_document document;
  wstring path = taiga::GetPath(taiga::kPathUserHistory);
  xml_node node_history = document.append_child(L"history");

  // Write items
  xml_node node_items = node_history.append_child(L"items");
  foreach_(it, items) {
    xml_node node_item = node_items.append_child(L"item");
    node_item.append_attribute(L"anime_id") = it->anime_id;
    node_item.append_attribute(L"episode") = *it->episode;
    node_item.append_attribute(L"time") = it->time.c_str();
  }
  // Write queue
  xml_node node_queue = node_history.append_child(L"queue");
  foreach_(it, queue.items) {
    xml_node node_item = node_queue.append_child(L"item");
    #define APPEND_ATTRIBUTE_INT(x, y) \
        if (y) node_item.append_attribute(x) = *y;
    #define APPEND_ATTRIBUTE_STR(x, y) \
        if (y) node_item.append_attribute(x) = (*y).c_str();
    #define APPEND_ATTRIBUTE_DATE(x, y) \
        if (y) node_item.append_attribute(x) = wstring(*y).c_str();
    node_item.append_attribute(L"anime_id") = it->anime_id;
    node_item.append_attribute(L"mode") = it->mode;
    node_item.append_attribute(L"time") = it->time.c_str();
    APPEND_ATTRIBUTE_INT(L"episode", it->episode);
    APPEND_ATTRIBUTE_INT(L"score", it->score);
    APPEND_ATTRIBUTE_INT(L"status", it->status);
    APPEND_ATTRIBUTE_INT(L"enable_rewatching", it->enable_rewatching);
    APPEND_ATTRIBUTE_STR(L"tags", it->tags);
    APPEND_ATTRIBUTE_DATE(L"date_start", it->date_start);
    APPEND_ATTRIBUTE_DATE(L"date_finish", it->date_finish);
    #undef APPEND_ATTRIBUTE_DATE
    #undef APPEND_ATTRIBUTE_STR
    #undef APPEND_ATTRIBUTE_INT
  }

  return XmlWriteDocumentToFile(document, path);
}

////////////////////////////////////////////////////////////////////////////////

ConfirmationQueue::ConfirmationQueue()
    : in_process_(false) {
}

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
      auto anime_item = AnimeDatabase.FindItem(episode.anime_id);
      AddToQueue(*anime_item, episode, change_status);
    }
    queue_.pop();
  }

  in_process_ = false;
}