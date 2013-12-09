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
#include "base/foreach.h"

#include "anime_db.h"
#include "anime_episode.h"
#include "anime_item.h"
#include "anime_util.h"

#include "base/common.h"
#include "base/file.h"
#include "history.h"
#include "base/logger.h"
#include "taiga/settings.h"
#include "base/string.h"
#include "taiga/taiga.h"
#include "base/time.h"

#include "ui/dlg/dlg_anime_list.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_search.h"

#include "win/win_taskbar.h"

anime::Database* anime::Item::database_ = &AnimeDatabase;

namespace anime {

// =============================================================================

Item::Item()
    : keep_title(false), 
      last_modified(0), 
      my_info_(nullptr) {
}

Item::~Item() {
}

// =============================================================================

int Item::GetId() const {
  return metadata_.uid;
}

int Item::GetType() const {
  return metadata_.type;
}

int Item::GetEpisodeCount() const {
  if (metadata_.extent.size() > 0)
    return metadata_.extent.at(0);

  return -1;
}

int Item::GetEpisodeLength() const {
  if (metadata_.extent.size() > 1)
    return metadata_.extent.at(1);

  return -1;
}

int Item::GetAiringStatus(bool check_date) const {
  if (!check_date) return metadata_.status;
  if (IsFinishedAiring()) return kFinishedAiring;
  if (IsAiredYet()) return kAiring;
  return kNotYetAired;
}

const wstring& Item::GetTitle() const {
  return metadata_.title;
}

const wstring& Item::GetEnglishTitle(bool fallback) const {
  foreach_(it, metadata_.alternative)
    if (it->type == library::kTitleTypeLangEnglish)
      return it->value;

  if (fallback)
    return metadata_.title;

  return EmptyString();
}

vector<wstring> Item::GetSynonyms() const {
  vector<wstring> synonyms;

  foreach_(it, metadata_.alternative)
    if (it->type == library::kTitleTypeSynonym)
      synonyms.push_back(it->value);
  
  return synonyms;
}

const Date& Item::GetDateStart() const {
  if (metadata_.date.size() > 0)
    return metadata_.date.at(0);

  return EmptyDate();
}

const Date& Item::GetDateEnd() const {
  if (metadata_.date.size() > 1)
    return metadata_.date.at(1);

  return EmptyDate();
}

const wstring& Item::GetImageUrl() const {
  if (metadata_.resource.size() > 0)
    return metadata_.resource.at(0);

  return EmptyString();
}

const vector<wstring>& Item::GetGenres() const {
  return metadata_.subject;
}

const wstring& Item::GetPopularity() const {
  if (metadata_.community.size() > 1)
    return metadata_.community.at(1);

  return EmptyString();
}

const vector<wstring>& Item::GetProducers() const {
  return metadata_.creator;
}

const wstring& Item::GetScore() const {
  if (metadata_.community.size() > 0)
    return metadata_.community.at(0);

  return EmptyString();
}

const wstring& Item::GetSynopsis() const {
  return metadata_.description;
}

// =============================================================================

int Item::GetMyLastWatchedEpisode(bool check_events) const {
  if (!my_info_.get()) return 0;
  EventItem* event_item = check_events ? 
    SearchHistory(EVENT_SEARCH_EPISODE) : nullptr;
  return event_item ? *event_item->episode : my_info_->watched_episodes;
}

int Item::GetMyScore(bool check_events) const {
  if (!my_info_.get()) return 0;
  EventItem* event_item = check_events ? 
    SearchHistory(EVENT_SEARCH_SCORE) : nullptr;
  return event_item ? *event_item->score : my_info_->score;
}

int Item::GetMyStatus(bool check_events) const {
  if (!my_info_.get()) return kNotInList;
  EventItem* event_item = check_events ? 
    SearchHistory(EVENT_SEARCH_STATUS) : nullptr;
  return event_item ? *event_item->status : my_info_->status;
}

int Item::GetMyRewatching(bool check_events) const {
  if (!my_info_.get()) return FALSE;
  EventItem* event_item = check_events ? 
    SearchHistory(EVENT_SEARCH_REWATCH) : nullptr;
  return event_item ? *event_item->enable_rewatching : my_info_->rewatching;
}

int Item::GetMyRewatchingEp() const {
  if (!my_info_.get()) return 0;
  return my_info_->rewatching_ep;
}

const Date Item::GetMyDateStart(bool check_events) const {
  if (!my_info_.get()) return Date();
  EventItem* event_item = nullptr;
  event_item = check_events ? 
    SearchHistory(EVENT_SEARCH_DATE_START) : nullptr;
  return event_item ? 
    TranslateDateFromApi(*event_item->date_start) : 
    my_info_->date_start;
}

const Date Item::GetMyDateEnd(bool check_events) const {
  if (!my_info_.get()) return Date();
  EventItem* event_item = nullptr;
  event_item = check_events ? 
    SearchHistory(EVENT_SEARCH_DATE_END) : nullptr;
  return event_item ? 
    TranslateDateFromApi(*event_item->date_finish) : 
    my_info_->date_finish;
}

const wstring Item::GetMyLastUpdated() const {
  if (!my_info_.get()) return wstring();
  return my_info_->last_updated;
}

const wstring Item::GetMyTags(bool check_events) const {
  if (!my_info_.get()) return wstring();
  EventItem* event_item = check_events ? 
    SearchHistory(EVENT_SEARCH_TAGS) : nullptr;
  return event_item ? *event_item->tags : my_info_->tags;
}

// =============================================================================

void Item::SetId(int anime_id) {
  metadata_.uid = anime_id;
}

void Item::SetType(int type) {
  metadata_.type = type;
}

void Item::SetEpisodeCount(int number) {
  if (metadata_.extent.size() < 1)
    metadata_.extent.resize(1);

  metadata_.extent.at(0) = number;

  if (number >= 0)
    if (static_cast<size_t>(number) != local_info_.available_episodes.size())
      local_info_.available_episodes.resize(number);
}

void Item::SetEpisodeLength(int number) {
  if (metadata_.extent.size() < 2)
    metadata_.extent.resize(2);

  metadata_.extent.at(1) = number;
}

void Item::SetAiringStatus(int status) {
  metadata_.status = status;
}

void Item::SetTitle(const wstring& title) {
  metadata_.title = title;
}

void Item::SetEnglishTitle(const wstring& title) {
  foreach_(it, metadata_.alternative) {
    if (it->type == library::kTitleTypeLangEnglish) {
      it->value = title;
      return;
    }
  }

  library::Title new_title;
  new_title.type = library::kTitleTypeLangEnglish;
  new_title.value = title;
  metadata_.alternative.push_back(new_title);
}

void Item::SetSynonyms(const wstring& synonyms) {
  vector<wstring> temp;
  Split(synonyms, L"; ", temp);
  RemoveEmptyStrings(temp);
  SetSynonyms(temp);
}

void Item::SetSynonyms(const vector<wstring>& synonyms) {
  std::vector<library::Title> alternative;

  foreach_(it, metadata_.alternative)
    if (it->type != library::kTitleTypeSynonym)
      alternative.push_back(*it);

  foreach_(it, synonyms) {
    library::Title new_title;
    new_title.type = library::kTitleTypeSynonym;
    new_title.value = *it;
    alternative.push_back(new_title);
  }

  metadata_.alternative = alternative;
}

void Item::SetDateStart(const Date& date) {
  if (metadata_.date.size() < 1)
    metadata_.date.resize(1);

  metadata_.date.at(0) = date;
}

void Item::SetDateEnd(const Date& date) {
  if (metadata_.date.size() < 2)
    metadata_.date.resize(2);

  metadata_.date.at(1) = date;
}

void Item::SetImageUrl(const wstring& url) {
  if (metadata_.resource.size() < 1)
    metadata_.resource.resize(1);

  metadata_.resource.at(0) = url;
}

void Item::SetGenres(const wstring& genres) {
  vector<wstring> temp;
  Split(genres, L", ", temp);
  RemoveEmptyStrings(temp);
  SetGenres(temp);
}

void Item::SetGenres(const vector<wstring>& genres) {
  metadata_.subject = genres;
}

void Item::SetPopularity(const wstring& popularity) {
  if (metadata_.community.size() < 2)
    metadata_.community.resize(2);

  metadata_.community.at(1) = popularity;
}

void Item::SetProducers(const wstring& producers) {
  vector<wstring> temp;
  Split(producers, L", ", temp);
  RemoveEmptyStrings(temp);
  SetProducers(temp);
}

void Item::SetProducers(const vector<wstring>& producers) {
  metadata_.creator = producers;
}

void Item::SetScore(const wstring& score) {
  if (metadata_.community.size() < 1)
    metadata_.community.resize(1);

  metadata_.community.at(0) = score;
}

void Item::SetSynopsis(const wstring& synopsis) {
  metadata_.description = synopsis;
}

// =============================================================================

void Item::SetMyLastWatchedEpisode(int number) {
  assert(my_info_.get());
  my_info_->watched_episodes = number;
}

void Item::SetMyScore(int score) {
  assert(my_info_.get());
  my_info_->score = score;
}

void Item::SetMyStatus(int status) {
  assert(my_info_.get());
  my_info_->status = status;
}

void Item::SetMyRewatching(int rewatching) {
  assert(my_info_.get());
  my_info_->rewatching = rewatching;
}

void Item::SetMyRewatchingEp(int rewatching_ep) {
  assert(my_info_.get());
  my_info_->rewatching_ep = rewatching_ep;
}

void Item::SetMyDateStart(const Date& date) {
  assert(my_info_.get());
  my_info_->date_start = date;
}

void Item::SetMyDateEnd(const Date& date) {
  assert(my_info_.get());
  my_info_->date_finish = date;
}

void Item::SetMyLastUpdated(const wstring& last_updated) {
  assert(my_info_.get());
  my_info_->last_updated = last_updated;
}

void Item::SetMyTags(const wstring& tags) {
  assert(my_info_.get());
  my_info_->tags = tags;
}

// =============================================================================

bool Item::IsAiredYet() const {
  if (metadata_.status != kNotYetAired) return true;
  if (!IsValidDate(GetDateStart())) return false;
  
  Date date_japan = GetDateJapan();
  Date date_start = GetDateStart();
  
  // Assume the worst case
  if (!date_start.month)
    date_start.month = 12;
  if (!date_start.day)
    date_start.day = 31;
  
  return date_japan >= date_start;
}

bool Item::IsFinishedAiring() const {
  if (metadata_.status == kFinishedAiring) return true;
  if (!IsValidDate(GetDateEnd())) return false;
  if (!IsAiredYet()) return false;
  return GetDateJapan() > GetDateEnd();
}

// =============================================================================

bool Item::CheckEpisodes(int number, bool check_folder) {
  // Check folder
  if (check_folder)
    CheckFolder();
  if (GetFolder().empty()) {
    for (int i = 1; i <= GetAvailableEpisodeCount(); i++)
      SetEpisodeAvailability(i, false, L"");
    return false;
  }
  
  // Check all episodes
  if (number == -1) {
    SearchFileFolder(*this, GetFolder(), -1, false);
    return true;

  // Check single episode
  } else {
    if (number == 0) {
      if (IsNewEpisodeAvailable()) return true;
      SetNewEpisodePath(L"");
      number = GetEpisodeCount() == 1 ? 0 : GetMyLastWatchedEpisode() + 1;
    }
    wstring file = SearchFileFolder(*this, GetFolder(), number, false);
    return !file.empty();
  }
}

int Item::GetAvailableEpisodeCount() const {
  return static_cast<int>(local_info_.available_episodes.size());
}

int Item::GetLastAiredEpisodeNumber(bool estimate) {
  if (local_info_.last_aired_episode)
    return local_info_.last_aired_episode;

  // No need to estimate if the series isn't currently airing
  switch (GetAiringStatus()) {
    case kFinishedAiring:
      local_info_.last_aired_episode = GetEpisodeCount();
      return local_info_.last_aired_episode;
    case kNotYetAired:
    case kUnknownStatus:
      return 0;
  }
  
  if (!estimate)
    return 0;

  // Can't estimate for other types of anime
  if (GetType() != kTv)
    return 0;

  // TV series air weekly, so the number of weeks that has passed since the day
  // the series started airing gives us the last aired episode. Note that
  // irregularities such as broadcasts being postponed due to sports events make
  // this method unreliable.
  const Date& date_start = GetDateStart();
  if (date_start.year && date_start.month && date_start.day) { 
    // To compensate for the fact that we don't know the airing hour,
    // we substract one more day.
    int date_diff = GetDateJapan() - date_start - 1;
    if (date_diff > -1) {
      int number_of_weeks = date_diff / 7;
      if (number_of_weeks < GetEpisodeCount()) {
        local_info_.last_aired_episode = number_of_weeks + 1;
      } else {
        local_info_.last_aired_episode = GetEpisodeCount();
      }
    }
  }

  return local_info_.last_aired_episode;
}

wstring Item::GetNewEpisodePath() const {
  return local_info_.new_episode_path;
}

bool Item::IsEpisodeAvailable(int number) const {
  if (number < 1) number = 1;
  if (static_cast<size_t>(number) > local_info_.available_episodes.size())
    return false;

  return local_info_.available_episodes.at(number - 1);
}

bool Item::IsNewEpisodeAvailable() const {
  return IsEpisodeAvailable(GetMyLastWatchedEpisode() + 1);
}

bool Item::PlayEpisode(int number) {
  if (number > GetEpisodeCount() && GetEpisodeCount() != 0)
    return false;

  wstring file_path;

  SetSharedCursor(IDC_WAIT);

  // Check saved episode path
  if (number == GetMyLastWatchedEpisode() + 1)
    if (!GetNewEpisodePath().empty())
      if (FileExists(GetNewEpisodePath()))
        file_path = GetNewEpisodePath();
  
  // Check anime folder
  if (file_path.empty()) {
    CheckFolder();
    if (!GetFolder().empty()) {
      file_path = SearchFileFolder(*this, GetFolder(), number, false);
    }
  }

  // Check other folders
  if (file_path.empty()) {
    for (auto it = Settings.Folders.root.begin(); 
         file_path.empty() && it != Settings.Folders.root.end(); ++it) {
      file_path = SearchFileFolder(*this, *it, number, false);
    }
  }

  if (file_path.empty()) {
    if (number == 0) number = 1;
    MainDialog.ChangeStatus(
      L"Could not find episode #" + ToWstr(number) + L" (" + GetTitle() + L").");
  } else {
    Execute(file_path);
  }

  SetSharedCursor(IDC_ARROW);

  return !file_path.empty();
}

bool Item::SetEpisodeAvailability(int number, bool available, const wstring& path) {
  if (number == 0) number = 1;
  
  if (number <= GetEpisodeCount() || GetEpisodeCount() == 0) {
    if (static_cast<size_t>(number) > local_info_.available_episodes.size()) {
      local_info_.available_episodes.resize(number);
    }
    local_info_.available_episodes.at(number - 1) = available;
    if (number == GetMyLastWatchedEpisode() + 1) {
      SetNewEpisodePath(path);
    }
    int list_index = AnimeListDialog.GetListIndex(GetId());
    if (list_index > -1) {
      AnimeListDialog.listview.RedrawItems(list_index, list_index, true);
    }
    return true;
  }
  
  return false;
}

void Item::SetLastAiredEpisodeNumber(int number) {
  if (number > local_info_.last_aired_episode)
    local_info_.last_aired_episode = number;
}

void Item::SetNewEpisodePath(const wstring& path) {
  local_info_.new_episode_path = path;
}

// =============================================================================

bool Item::CheckFolder() {
  // Check if current folder still exists
  if (!GetFolder().empty() && !FolderExists(GetFolder())) {
    LOG(LevelWarning, L"Folder doesn't exist anymore.");
    LOG(LevelWarning, L"Path: " + GetFolder());
    SetFolder(L"");
  }

  // Search root folders
  if (GetFolder().empty()) {
    wstring new_folder;
    for (auto it = Settings.Folders.root.begin(); 
         it != Settings.Folders.root.end(); ++it) {
      new_folder = SearchFileFolder(*this, *it, 0, true);
      if (!new_folder.empty()) {
        SetFolder(new_folder);
        Settings.Save();
        break;
      }
    }
  }

  return !GetFolder().empty();
}

const wstring Item::GetFolder() const {
  return local_info_.folder;
}

void Item::SetFolder(const wstring& folder) {
  local_info_.folder = folder;
}

// =============================================================================

bool Item::GetPlaying() const {
  return local_info_.playing;
}

void Item::SetPlaying(bool playing) {
  local_info_.playing = playing;
}

// =============================================================================

const vector<wstring>& Item::GetUserSynonyms() const {
  return local_info_.synonyms;
}

void Item::SetUserSynonyms(const wstring& synonyms) {
  vector<wstring> temp;
  Split(synonyms, L"; ", temp);
  SetUserSynonyms(temp);
}

void Item::SetUserSynonyms(const vector<wstring>& synonyms) {
  local_info_.synonyms = synonyms;
  RemoveEmptyStrings(local_info_.synonyms);

  if (!synonyms.empty() && CurrentEpisode.anime_id == anime::ID_NOTINLIST) {
    CurrentEpisode.Set(anime::ID_UNKNOWN);
  }
}

bool Item::UserSynonymsAvailable() const {
  return !local_info_.synonyms.empty();
}

bool Item::GetUseAlternative() const {
  return local_info_.use_alternative;
}

void Item::SetUseAlternative(bool use_alternative) {
  local_info_.use_alternative = use_alternative;
}

// =============================================================================

void Item::AddtoUserList() {
  if (!my_info_.get()) {
    my_info_.reset(new MyInformation);
  }
}

bool Item::IsInList() const {
  return my_info_.get() && GetMyStatus() != kNotInList;
}

void Item::RemoveFromUserList() {
  assert(my_info_.use_count() <= 1);
  my_info_.reset();
  assert(my_info_.use_count() == 0);
}

bool Item::IsOldEnough() const {
  if (!last_modified) return true;

  time_t time_diff = time(nullptr) - last_modified;

  if (GetAiringStatus() == kFinishedAiring) {
    return time_diff >= 60 * 60 * 24 * 7; // 1 week
  } else {
    return time_diff >= 60 * 60; // 1 hour
  }
}

// =============================================================================

void Item::Edit(const EventItem& item) {
  // Edit episode
  if (item.episode) {
    SetMyLastWatchedEpisode(*item.episode);
  }
  // Edit score
  if (item.score) {
    SetMyScore(*item.score);
  }
  // Edit status
  if (item.status) {
    SetMyStatus(*item.status);
  }
  // Edit re-watching status
  if (item.enable_rewatching) {
    SetMyRewatching(*item.enable_rewatching);
  }
  // Edit tags
  if (item.tags) {
    SetMyTags(*item.tags);
  }
  // Edit dates
  if (item.date_start) {
    Date date_start = TranslateDateFromApi(*item.date_start);
    SetMyDateStart(date_start);
  }
  if (item.date_finish) {
    Date date_finish = TranslateDateFromApi(*item.date_finish);
    SetMyDateEnd(date_finish);
  }
  // Delete
  if (item.mode == taiga::kHttpServiceDeleteLibraryEntry) {
    MainDialog.ChangeStatus(L"Item deleted. (" + GetTitle() + L")");
    database_->DeleteListItem(GetId());
    AnimeListDialog.RefreshList();
    AnimeListDialog.RefreshTabs();
    SearchDialog.RefreshList();
    if (CurrentEpisode.anime_id == item.anime_id) {
      CurrentEpisode.Set(anime::ID_NOTINLIST);
    }
  }

  // Save list
  database_->SaveList();

  // Remove item from event queue
  History.queue.Remove();
  // Check for more events
  History.queue.Check(false);

  // Redraw main list item
  int list_index = AnimeListDialog.GetListIndex(GetId());
  if (list_index > -1) {
    AnimeListDialog.listview.RedrawItems(list_index, list_index, true);
  }
}

// =============================================================================

EventItem* Item::SearchHistory(int search_mode) const {
  return History.queue.FindItem(metadata_.uid, search_mode);
}

} // namespace anime