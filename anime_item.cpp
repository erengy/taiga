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

#include "std.h"

#include "anime_db.h"
#include "anime_episode.h"
#include "anime_item.h"

#include "common.h"
#include "history.h"
#include "myanimelist.h"
#include "recognition.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "time.h"

#include "dlg/dlg_anime_list.h"
#include "dlg/dlg_main.h"
#include "dlg/dlg_search.h"

#include "win32/win_taskbar.h"

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
  return series_info_.id;
}

int Item::GetType() const {
  return series_info_.type;
}

int Item::GetEpisodeCount(bool estimation) const {
  // Generally we want an exact number without estimation
  if (!estimation || series_info_.episodes > 0)
    return series_info_.episodes;

  int number = 0;

  // Estimate using user information
  if (IsInList())
    number = max(GetMyLastWatchedEpisode(), GetAvailableEpisodeCount());

  // Estimate using airing dates of TV series
  if (series_info_.type == mal::TYPE_TV) {
    Date date_start = GetDate(DATE_START);
    if (mal::IsValidDate(date_start)) {
      Date date_end = GetDate(DATE_END);
      // Use current date in Japan if ending date is unknown
      if (!mal::IsValidDate(date_end)) date_end = GetDateJapan();
      // Assuming the series is aired weekly
      number = max(number, (date_end - date_start) / 7);
    }
  }

  // Given all TV series aired since 2000, most them have their episodes
  // spanning one or two seasons. Following is a table of top ten values:
  //
  //   Episodes    Seasons    Percent
  //   ------------------------------
  //         12          1      23.6%
  //         13          1      20.2%
  //         26          2      15.4%
  //         24          2       6.4%
  //         25          2       5.0%
  //         52          4       4.4%
  //         51          4       3.1%
  //         11          1       2.6%
  //         50          4       2.3%
  //         39          3       1.4%
  //   ------------------------------
  //   Total:                   84.6%
  //
  // With that in mind, we can normalize our output at several points.
  if (number < 12) return 13;
  if (number < 24) return 26;
  if (number < 50) return 52;
  
  // This is a series that has aired for more than a year, which means we cannot
  // estimate for how long it is going to continue.
  return 0;
}

int Item::GetAiringStatus(bool check_date) const {
  if (!check_date) return series_info_.status;
  if (IsFinishedAiring()) return mal::STATUS_FINISHED;
  if (IsAiredYet()) return mal::STATUS_AIRING;
  return mal::STATUS_NOTYETAIRED;
}

const wstring& Item::GetTitle() const {
  return series_info_.title;
}

const wstring& Item::GetEnglishTitle(bool fallback) const {
  if (series_info_.english_title.empty() && fallback)
    return series_info_.title;
  return series_info_.english_title;
}

const vector<wstring>& Item::GetSynonyms() const {
  return series_info_.synonyms;
}

const Date& Item::GetDate(DateType type) const {
  switch (type) {
    case DATE_START:
    default:
      return series_info_.date_start;
    case DATE_END:
      return series_info_.date_end;
  }
}

const wstring& Item::GetImageUrl() const {
  return series_info_.image_url;
}

const wstring& Item::GetGenres() const {
  return series_info_.genres;
}

const wstring& Item::GetPopularity() const {
  return series_info_.popularity;
}

const wstring& Item::GetProducers() const {
  return series_info_.producers;
}

const wstring& Item::GetRank() const {
  return series_info_.rank;
}

const wstring& Item::GetScore() const {
  return series_info_.score;
}

const wstring& Item::GetSynopsis() const {
  return series_info_.synopsis;
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
  if (!my_info_.get()) return mal::MYSTATUS_NOTINLIST;
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

const Date Item::GetMyDate(DateType type, bool check_events) const {
  if (!my_info_.get()) return Date();
  EventItem* event_item = nullptr;
  switch (type) {
    case DATE_START:
    default:
      event_item = check_events ? 
        SearchHistory(EVENT_SEARCH_DATE_START) : nullptr;
      return event_item ? 
        mal::TranslateDateFromApi(*event_item->date_start) : 
        my_info_->date_start;
    case DATE_END:
      event_item = check_events ? 
        SearchHistory(EVENT_SEARCH_DATE_END) : nullptr;
      return event_item ? 
        mal::TranslateDateFromApi(*event_item->date_finish) : 
        my_info_->date_finish;
  }
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
  series_info_.id = anime_id;
}

void Item::SetType(int type) {
  series_info_.type = type;
}

void Item::SetEpisodeCount(int number) {
  series_info_.episodes = number;
}

void Item::SetAiringStatus(int status) {
  series_info_.status = status;
}

void Item::SetTitle(const wstring& title) {
  series_info_.title = title;
}

void Item::SetEnglishTitle(const wstring& title) {
  series_info_.english_title = title;
}

void Item::SetSynonyms(const wstring& synonyms) {
  vector<wstring> temp;
  Split(synonyms, L"; ", temp);
  SetSynonyms(temp);
}

void Item::SetSynonyms(const vector<wstring>& synonyms) {
  series_info_.synonyms = synonyms;
  RemoveEmptyStrings(series_info_.synonyms);
}

void Item::SetDate(DateType type, const Date& date) {
  switch (type) {
    case DATE_START:
      series_info_.date_start = date;
      break;
    case DATE_END:
      series_info_.date_end = date;
      break;
  }
}

void Item::SetImageUrl(const wstring& url) {
  series_info_.image_url = url;
}

void Item::SetGenres(const wstring& genres) {
  series_info_.genres = genres;
}

void Item::SetPopularity(const wstring& popularity) {
  series_info_.popularity = popularity;
}

void Item::SetProducers(const wstring& producers) {
  series_info_.producers = producers;
}

void Item::SetRank(const wstring& rank) {
  series_info_.rank = rank;
}

void Item::SetScore(const wstring& score) {
  series_info_.score = score;
}

void Item::SetSynopsis(const wstring& synopsis) {
  series_info_.synopsis = synopsis;
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

void Item::SetMyDate(DateType type, const Date& date) {
  assert(my_info_.get());
  switch (type) {
    case DATE_START:
      my_info_->date_start = date;
      break;
    case DATE_END:
      my_info_->date_finish = date;
      break;
  }
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
  if (series_info_.status != mal::STATUS_NOTYETAIRED) return true;
  if (!mal::IsValidDate(series_info_.date_start)) return false;
  
  Date date_japan = GetDateJapan();
  Date date_start = series_info_.date_start;
  
  // Assume the worst case
  if (!series_info_.date_start.month)
    date_start.month = 12;
  if (!series_info_.date_start.day)
    date_start.day = 31;
  
  return date_japan >= date_start;
}

bool Item::IsFinishedAiring() const {
  if (series_info_.status == mal::STATUS_FINISHED) return true;
  if (!mal::IsValidDate(series_info_.date_end)) return false;
  if (!IsAiredYet()) return false;
  return GetDateJapan() > series_info_.date_end;
}

// =============================================================================

bool Item::CheckEpisodes(int number, bool check_folder) {
  // Check folder
  if (check_folder) CheckFolder();
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
  if (!my_info_.get()) return 0;
  return static_cast<int>(my_info_->available_episodes.size());
}

int Item::GetLastAiredEpisodeNumber(bool estimate) const {
  if (!my_info_.get())
    return 0;

  if (my_info_->last_aired_episode)
    return my_info_->last_aired_episode;

  // No need to estimate if the series isn't currently airing
  switch (GetAiringStatus()) {
    case mal::STATUS_FINISHED:
      my_info_->last_aired_episode = GetEpisodeCount();
      return my_info_->last_aired_episode;
    case mal::STATUS_NOTYETAIRED:
    case mal::STATUS_UNKNOWN:
      return 0;
  }
  
  if (!estimate)
    return 0;

  // Can't estimate for other types of anime
  if (GetType() != mal::TYPE_TV)
    return 0;

  // TV series air weekly, so the number of weeks that has passed since the day
  // the series started airing gives us the last aired episode. Note that
  // irregularities such as broadcasts being postponed due to sports events make
  // this method unreliable.
  const Date& date_start = GetDate(anime::DATE_START);
  if (date_start.year && date_start.month && date_start.day) { 
    // To compensate for the fact that we don't know the airing hour,
    // we substract one more day.
    int date_diff = GetDateJapan() - date_start - 1;
    if (date_diff > -1) {
      int number_of_weeks = date_diff / 7;
      if (number_of_weeks < GetEpisodeCount()) {
        my_info_->last_aired_episode = number_of_weeks + 1;
      } else {
        my_info_->last_aired_episode = GetEpisodeCount();
      }
    }
  }

  return my_info_->last_aired_episode;
}

wstring Item::GetNewEpisodePath() const {
  if (!my_info_.get()) return wstring();
  return my_info_->new_episode_path;
}

bool Item::IsEpisodeAvailable(int number) const {
  if (!my_info_.get()) return false;

  if (number < 1) number = 1;
  if (static_cast<size_t>(number) > my_info_->available_episodes.size())
    return false;

  return my_info_->available_episodes.at(number - 1);
}

bool Item::IsNewEpisodeAvailable() const {
  return IsEpisodeAvailable(GetMyLastWatchedEpisode() + 1);
}

bool Item::PlayEpisode(int number) {
  if (number > GetEpisodeCount() && GetEpisodeCount() != 0)
    return false;

  wstring file_path;

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
  return !file_path.empty();
}

bool Item::SetEpisodeAvailability(int number, bool available, const wstring& path) {
  assert(my_info_.get());

  if (number == 0) number = 1;
  
  if (number <= GetEpisodeCount() || GetEpisodeCount() == 0) {
    if (static_cast<size_t>(number) > my_info_->available_episodes.size()) {
      my_info_->available_episodes.resize(number);
    }
    my_info_->available_episodes.at(number - 1) = available;
    if (number == GetMyLastWatchedEpisode() + 1) {
      SetNewEpisodePath(path);
    }
    if (!my_info_->playing) {
      int list_index = AnimeListDialog.GetListIndex(GetId());
      if (list_index > -1) {
        AnimeListDialog.listview.RedrawItems(list_index, list_index, true);
      }
    }
    return true;
  }
  
  return false;
}

void Item::SetLastAiredEpisodeNumber(int number) {
  assert(my_info_.get());
  if (number > my_info_->last_aired_episode)
    my_info_->last_aired_episode = number;
}

void Item::SetNewEpisodePath(const wstring& path) {
  assert(my_info_.get());
  my_info_->new_episode_path = path;
}

// =============================================================================

bool Item::CheckFolder() {
  // Check if current folder still exists
  if (!GetFolder().empty() && !FolderExists(GetFolder())) {
    SetFolder(L"", false);
  }

  // Search root folders
  if (GetFolder().empty()) {
    wstring new_folder;
    for (auto it = Settings.Folders.root.begin(); 
         it != Settings.Folders.root.end(); ++it) {
      new_folder = SearchFileFolder(*this, *it, 0, true);
      if (!new_folder.empty()) {
        SetFolder(new_folder, true);
        break;
      }
    }
  }

  return !GetFolder().empty();
}

const wstring Item::GetFolder() const {
  if (!my_info_.get()) return wstring();
  return my_info_->folder;
}

void Item::SetFolder(const wstring& folder, bool save_settings) {
  assert(my_info_.get());

  if (folder == my_info_->folder) return;

  my_info_->folder = folder;

  if (save_settings) {
    Settings.Anime.SetItem(GetId(), folder, Optional<wstring>());
    Settings.Save();
  }
}

// =============================================================================

bool Item::GetPlaying() const {
  if (!my_info_.get()) return false;
  return my_info_->playing;
}

void Item::SetPlaying(bool playing) {
  assert(my_info_.get());
  my_info_->playing = playing;
}

// =============================================================================

const vector<wstring>& Item::GetUserSynonyms() const {
  assert(my_info_.get());
  return my_info_->synonyms;
}

void Item::SetUserSynonyms(const wstring& synonyms, bool save_settings) {
  vector<wstring> temp;
  Split(synonyms, L"; ", temp);
  SetUserSynonyms(temp, save_settings);
}

void Item::SetUserSynonyms(const vector<wstring>& synonyms, bool save_settings) {
  assert(my_info_.get());

  my_info_->synonyms = synonyms;
  RemoveEmptyStrings(my_info_->synonyms);

  Meow.UpdateCleanTitles(GetId());

  if (save_settings) {
    Settings.Anime.SetItem(GetId(), Optional<wstring>(), Join(my_info_->synonyms, L"; "));
    Settings.Save();
  }

  if (!synonyms.empty() && CurrentEpisode.anime_id == anime::ID_NOTINLIST) {
    CurrentEpisode.Set(anime::ID_UNKNOWN);
  }
}

// =============================================================================

void Item::AddtoUserList() {
  if (!my_info_.get()) {
    my_info_.reset(new MyInformation);
  }
}

bool Item::IsInList() const {
  return my_info_.get() && GetMyStatus() != mal::MYSTATUS_NOTINLIST;
}

void Item::RemoveFromUserList() {
  assert(my_info_.use_count() <= 1);
  my_info_.reset();
  assert(my_info_.use_count() == 0);
}

bool Item::IsOldEnough() const {
  if (!last_modified) return true;

  time_t time_diff = time(nullptr) - last_modified;

  if (GetAiringStatus() == mal::STATUS_FINISHED) {
    return time_diff >= 60 * 60 * 24 * 7; // 1 week
  } else {
    return time_diff >= 60 * 60; // 1 hour
  }
}

// =============================================================================

bool Item::Edit(EventItem& item, const wstring& data, int status_code) {
  if (!mal::UpdateSucceeded(item, data, status_code)) {
    // Show balloon tip on failure
    wstring text = L"Title: " + GetTitle();
    text += L"\nReason: " + (item.reason.empty() ? L"Unknown" : item.reason);
    text += L"\nClick to try again.";
    Taiga.current_tip_type = TIPTYPE_UPDATEFAILED;
    Taskbar.Tip(L"", L"", 0);
    Taskbar.Tip(text.c_str(), L"Update failed", NIIF_ERROR);
    return false;
  }

  // Edit episode
  if (item.episode) {
    SetMyLastWatchedEpisode(*item.episode);
    database_->SaveList(GetId(), L"my_watched_episodes", ToWstr(*item.episode));
  }
  // Edit score
  if (item.score) {
    SetMyScore(*item.score);
    database_->SaveList(GetId(), L"my_score", ToWstr(*item.score));
  }
  // Edit status
  if (item.status) {
    database_->user.IncreaseItemCount(*item.status, false);
    database_->user.DecreaseItemCount(GetMyStatus(false), true);
    SetMyStatus(*item.status);
    database_->SaveList(GetId(), L"my_status", ToWstr(*item.status));
  }
  // Edit re-watching status
  if (item.enable_rewatching) {
    SetMyRewatching(*item.enable_rewatching);
    database_->SaveList(GetId(), L"my_rewatching", ToWstr(*item.enable_rewatching));
  }
  // Edit ID (Add)
  if (item.mode == HTTP_MAL_AnimeAdd) {
    if (IsNumeric(data)) {
      database_->SaveList(GetId(), L"my_id", data); // deprecated
    }
  }
  // Edit tags
  if (item.tags) {
    SetMyTags(*item.tags);
    database_->SaveList(GetId(), L"my_tags", *item.tags);
  }
  // Edit dates
  if (item.date_start) {
    Date date_start = mal::TranslateDateFromApi(*item.date_start);
    SetMyDate(anime::DATE_START, date_start);
    database_->SaveList(GetId(), L"my_start_date", date_start, EDIT_ANIME);
  }
  if (item.date_finish) {
    Date date_finish = mal::TranslateDateFromApi(*item.date_finish);
    SetMyDate(anime::DATE_END, date_finish);
    database_->SaveList(GetId(), L"my_finish_date", date_finish, EDIT_ANIME);
  }
  // Delete
  if (item.mode == HTTP_MAL_AnimeDelete) {
    MainDialog.ChangeStatus(L"Item deleted. (" + GetTitle() + L")");
    database_->DeleteListItem(GetId());
    AnimeListDialog.RefreshList();
    AnimeListDialog.RefreshTabs();
    SearchDialog.RefreshList();
    if (CurrentEpisode.anime_id == item.anime_id) {
      CurrentEpisode.Set(anime::ID_NOTINLIST);
    }
  }

  // Remove item from event queue
  History.queue.Remove();
  // Check for more events
  History.queue.Check();

  // Redraw main list item
  int list_index = AnimeListDialog.GetListIndex(GetId());
  if (list_index > -1) {
    AnimeListDialog.listview.RedrawItems(list_index, list_index, true);
  }

  return true;
}

// =============================================================================

EventItem* Item::SearchHistory(int search_mode) const {
  return History.queue.FindItem(series_info_.id, search_mode);
}

} // namespace anime