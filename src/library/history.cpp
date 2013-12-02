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

#include "base/std.h"

#include "history.h"

#include "anime_db.h"
#include "taiga/announce.h"
#include "base/common.h"
#include "taiga/http.h"
#include "base/logger.h"
#include "sync/myanimelist.h"
#include "sync/sync.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "base/string.h"
#include "taiga/taiga.h"
#include "base/xml.h"

#include "ui/dlg/dlg_anime_info.h"
#include "ui/dlg/dlg_anime_list.h"
#include "ui/dlg/dlg_history.h"
#include "ui/dlg/dlg_main.h"

#include "win/win_taskdialog.h"

class ConfirmationQueue ConfirmationQueue;
class History History;

// =============================================================================

EventItem::EventItem()
    : anime_id(anime::ID_UNKNOWN),
      enabled(true),
      mode(0) {
}

EventQueue::EventQueue()
    : index(0),
      history(nullptr),
      updating(false) {
}

void EventQueue::Add(EventItem& item, bool save) {
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
      if (anime->GetMyDate(anime::DATE_START) == sync::myanimelist::TranslateDateFromApi(*item.date_start))
        item.date_start.Reset();
    if (item.date_finish)
      if (anime->GetMyDate(anime::DATE_END) == sync::myanimelist::TranslateDateFromApi(*item.date_finish))
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
          !item.date_finish) return;
      break;
  }

  // Edit previous item with the same ID...
  bool add_new_item = true;
  if (!History.queue.updating) {
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
      if (it->anime_id == item.anime_id && it->enabled) {
        if (it->mode != taiga::kHttpServiceAddLibraryEntry && it->mode != taiga::kHttpServiceDeleteLibraryEntry) {
          if (!item.episode || (!it->episode && it == items.rbegin())) {
            if (item.episode) it->episode = *item.episode;
            if (item.score) it->score = *item.score;
            if (item.status) it->status = *item.status;
            if (item.enable_rewatching) it->enable_rewatching = *item.enable_rewatching;
            if (item.tags) it->tags = *item.tags;
            if (item.date_start) it->date_start = *item.date_start;
            if (item.date_finish) it->date_finish = *item.date_finish;
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
      Taiga.play_status = PLAYSTATUS_UPDATED;
      Announcer.Do(ANNOUNCE_TO_HTTP | ANNOUNCE_TO_TWITTER, &episode);
    }

    // Check new episode
    if (item.episode) {
      anime->SetNewEpisodePath(L"");
      anime->CheckEpisodes(0);
    }
    
    // Refresh history
    MainDialog.treeview.RefreshHistoryCounter();
    HistoryDialog.RefreshList();

    // Refresh anime window
    if (item.mode == taiga::kHttpServiceAddLibraryEntry ||
        item.mode == taiga::kHttpServiceDeleteLibraryEntry || 
        item.status ||
        item.enable_rewatching) {
      AnimeListDialog.RefreshList();
      AnimeListDialog.RefreshTabs();
    } else {
      AnimeListDialog.RefreshListItem(item.anime_id);
    }

    // Refresh now playing
    NowPlayingDialog.Refresh(false, false, false);
    
    // Change status
    if (!Taiga.logged_in) {
      MainDialog.ChangeStatus(L"\"" + anime->GetTitle() +
                              L"\" is queued for update.");
    }

    // Update
    Check();
  }
}

void EventQueue::Check(bool automatic) {
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
  if (automatic && !Settings.Program.General.enable_sync) {
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
  MainDialog.ChangeStatus(L"Updating list...");
  sync::myanimelist::AnimeValues* anime_values =
      static_cast<sync::myanimelist::AnimeValues*>(&items[index]);
  sync::UpdateLibraryEntry(*anime_values, items[index].anime_id,
      static_cast<taiga::HttpClientMode>(items[index].mode));
}

void EventQueue::Clear(bool save) {
  items.clear();
  index = 0;

  MainDialog.treeview.RefreshHistoryCounter();
  NowPlayingDialog.Refresh(false, false, false);

  if (save) history->Save();
}

EventItem* EventQueue::FindItem(int anime_id, int search_mode) {
  for (auto it = items.rbegin(); it != items.rend(); ++it) {
    if (it->anime_id == anime_id && it->enabled) {
      switch (search_mode) {
        // Date
        case EVENT_SEARCH_DATE_START:
          if (it->date_start) return &(*it);
          break;
        case EVENT_SEARCH_DATE_END:
          if (it->date_finish) return &(*it);
          break;
        // Episode
        case EVENT_SEARCH_EPISODE:
          if (it->episode) return &(*it);
          break;
        // Re-watching
        case EVENT_SEARCH_REWATCH:
          if (it->enable_rewatching) return &(*it);
          break;
        // Score
        case EVENT_SEARCH_SCORE:
          if (it->score) return &(*it);
          break;
        // Status
        case EVENT_SEARCH_STATUS:
          if (it->status) return &(*it);
          break;
        // Tags
        case EVENT_SEARCH_TAGS:
          if (it->tags) return &(*it);
          break;
        // Default
        default:
          return &(*it);
      }
    }
  }
  return nullptr;
}

EventItem* EventQueue::GetCurrentItem() {
  if (!items.empty()) {
    return &items.at(index);
  }
  return nullptr;
}

int EventQueue::GetItemCount() {
  int count = 0;
  for (auto it = items.begin(); it != items.end(); ++it)
    if (it->enabled) count++;
  return count;
}

void EventQueue::Remove(int index, bool save, bool refresh, bool to_history) {
  if (index == -1) index = this->index;
  
  if (index < static_cast<int>(items.size())) {
    auto event_item = items.begin() + index;
    
    if (to_history && event_item->episode) {
      history->items.push_back(*event_item);
      if (history->limit > 0 && static_cast<int>(history->items.size()) > history->limit) {
        history->items.erase(history->items.begin());
      }
    }
    
    items.erase(event_item);

    if (refresh) {
      MainDialog.treeview.RefreshHistoryCounter();
      NowPlayingDialog.Refresh(false, false, false);
      HistoryDialog.RefreshList();
    }
  }

  if (save) history->Save();
}

void EventQueue::RemoveDisabled(bool save, bool refresh) {
  bool needs_refresh = false;
  
  for (size_t i = 0; i < items.size(); i++) {
    if (!items.at(i).enabled) {
      items.erase(items.begin() + i);
      needs_refresh = true;
      i--;
    }
  }

  if (refresh && needs_refresh) {
    MainDialog.treeview.RefreshHistoryCounter();
    NowPlayingDialog.Refresh(false, false, false);
    HistoryDialog.RefreshList();
  }

  if (save) history->Save();
}

// =============================================================================

History::History()
    : limit(0) { // Limit of history items
  queue.history = this;
}

bool History::Load() {
  // Initialize
  wstring file = Taiga.GetDataPath() + L"user\\" + Settings.Account.MAL.user + L"\\history.xml";
  
  // Load XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());

  // Items
  items.clear();
  xml_node node_items = doc.child(L"history").child(L"items");
  for (xml_node item = node_items.child(L"item"); item; item = item.next_sibling(L"item")) {
    EventItem event_item;
    event_item.anime_id = item.attribute(L"anime_id").as_int(anime::ID_NOTINLIST);
    event_item.episode = item.attribute(L"episode").as_int();
    event_item.time = item.attribute(L"time").value();
    items.push_back(event_item);
  }
  // Queue events
  queue.items.clear();
  xml_node node_queue = doc.child(L"history").child(L"queue");
  for (xml_node item = node_queue.child(L"item"); item; item = item.next_sibling(L"item")) {
    EventItem event_item;
    event_item.anime_id = item.attribute(L"anime_id").as_int(anime::ID_NOTINLIST);
    event_item.mode = item.attribute(L"mode").as_int();
    event_item.time = item.attribute(L"time").value();
    #define READ_ATTRIBUTE_INT(x, y) \
      if (!item.attribute(y).empty()) x = item.attribute(y).as_int();
    #define READ_ATTRIBUTE_STR(x, y) \
      if (!item.attribute(y).empty()) x = item.attribute(y).as_string();
    READ_ATTRIBUTE_INT(event_item.episode, L"episode");
    READ_ATTRIBUTE_INT(event_item.score, L"score");
    READ_ATTRIBUTE_INT(event_item.status, L"status");
    READ_ATTRIBUTE_INT(event_item.enable_rewatching, L"enable_rewatching");
    READ_ATTRIBUTE_STR(event_item.tags, L"tags");
    READ_ATTRIBUTE_STR(event_item.date_start, L"date_start");
    READ_ATTRIBUTE_STR(event_item.date_finish, L"date_finish");
    #undef READ_ATTRIBUTE_STR
    #undef READ_ATTRIBUTE_INT
    queue.Add(event_item, false);
  }

  return result.status == pugi::status_ok;
}

bool History::Save() {
  // Initialize
  wstring file = Taiga.GetDataPath() + L"user\\" + Settings.Account.MAL.user + L"\\history.xml";
  xml_document doc;
  xml_node node_history = doc.append_child(L"history");

  // Write event items
  xml_node node_items = node_history.append_child(L"items");
  for (auto j = items.begin(); j != items.end(); ++j) {
    xml_node node_item = node_items.append_child(L"item");
    node_item.append_attribute(L"anime_id") = j->anime_id;
    node_item.append_attribute(L"episode") = *j->episode;
    node_item.append_attribute(L"time") = j->time.c_str();
  }
  // Write event queue
  xml_node node_queue = node_history.append_child(L"queue");
  for (auto j = queue.items.begin(); j != queue.items.end(); ++j) {
    xml_node node_item = node_queue.append_child(L"item");
    #define APPEND_ATTRIBUTE_INT(x, y) \
      if (y) node_item.append_attribute(x) = *y;
    #define APPEND_ATTRIBUTE_STR(x, y) \
      if (y) node_item.append_attribute(x) = (*y).c_str();
    node_item.append_attribute(L"anime_id") = j->anime_id;
    node_item.append_attribute(L"mode") = j->mode;
    node_item.append_attribute(L"time") = j->time.c_str();
    APPEND_ATTRIBUTE_INT(L"episode", j->episode);
    APPEND_ATTRIBUTE_INT(L"score", j->score);
    APPEND_ATTRIBUTE_INT(L"status", j->status);
    APPEND_ATTRIBUTE_INT(L"enable_rewatching", j->enable_rewatching);
    APPEND_ATTRIBUTE_STR(L"tags", j->tags);
    APPEND_ATTRIBUTE_STR(L"date_start", j->date_start);
    APPEND_ATTRIBUTE_STR(L"date_finish", j->date_finish);
    #undef APPEND_ATTRIBUTE_STR
    #undef APPEND_ATTRIBUTE_INT
  }

  // Save file
  return doc.save_file(file.c_str(), L"\x09", pugi::format_default | pugi::format_write_bom);
}

// =============================================================================

ConfirmationQueue::ConfirmationQueue()
    : in_process(false) {
}

void ConfirmationQueue::Add(const anime::Episode& episode) {
  queue_.push(episode);
}

int AskForConfirmation(anime::Episode& episode) {
  auto anime_item = AnimeDatabase.FindItem(episode.anime_id);

  // Set up dialog
  win::TaskDialog dlg;
  wstring title = L"Anime title: " + anime_item->GetTitle();
  dlg.SetWindowTitle(APP_TITLE);
  dlg.SetMainIcon(TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"Do you want to update your anime list?");
  dlg.SetContent(title.c_str());
  dlg.SetVerificationText(L"Don't ask again, update automatically");
  dlg.UseCommandLinks(true);

  // Get episode number
  int number = GetEpisodeHigh(episode.number);
  if (number == 0) number = 1;
  if (anime_item->GetEpisodeCount() == 1) episode.number = L"1";

  // Add buttons
  if (anime_item->GetEpisodeCount() == number) { // Completed
    dlg.AddButton(L"Update and move\n"
                  L"Update and set as completed", IDCANCEL);
  } else if (anime_item->GetMyStatus() != anime::kWatching) { // Watching
    dlg.AddButton(L"Update and move\n"
                  L"Update and set as watching", IDCANCEL);
  }
  wstring button = L"Update\n"
                   L"Update episode number from " +
                   ToWstr(anime_item->GetMyLastWatchedEpisode()) +
                   L" to " + ToWstr(number);
  dlg.AddButton(button.c_str(), IDYES);
  dlg.AddButton(L"Cancel\n"
                L"Don't update anything", IDNO);

  // Show dialog
  dlg.Show(g_hMain);
  if (dlg.GetVerificationCheck())
    Settings.Account.Update.ask_to_confirm = FALSE;
  return dlg.GetSelectedButtonID();
}

void ConfirmationQueue::Process() {
  if (in_process) return;
  in_process = true;

  while (!queue_.empty()) {
    anime::Episode& episode = queue_.front();
    int choice = AskForConfirmation(episode);
    if (choice != IDNO) {
      bool change_status = (choice == IDCANCEL);
      auto anime_item = AnimeDatabase.FindItem(episode.anime_id);
      anime_item->AddToQueue(episode, change_status);
    }
    queue_.pop();
  }

  in_process = false;
}