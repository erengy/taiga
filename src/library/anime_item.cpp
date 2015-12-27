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

#include <algorithm>
#include <assert.h>

#include "base/foreach.h"
#include "base/string.h"
#include "base/time.h"
#include "library/anime_db.h"
#include "library/anime_item.h"
#include "library/anime_util.h"
#include "library/history.h"
#include "sync/sync.h"
#include "ui/ui.h"

anime::Database* anime::Item::database_ = &AnimeDatabase;

namespace anime {

Item::Item() {
  metadata_.uid.resize(sync::kLastService + 1);
}

Item::~Item() {
}

////////////////////////////////////////////////////////////////////////////////

int Item::GetId() const {
  assert(!metadata_.uid.empty());

  return ToInt(metadata_.uid.at(0));
}

const std::wstring& Item::GetId(enum_t service) const {
  assert(metadata_.uid.size() > service);

  return metadata_.uid.at(service);
}

const std::wstring& Item::GetSlug() const {
  if (metadata_.resource.size() > 1)
    return metadata_.resource.at(1);

  return EmptyString();
}

enum_t Item::GetSource() const {
  return metadata_.source;
}

int Item::GetType() const {
  return metadata_.type;
}

int Item::GetEpisodeCount() const {
  if (metadata_.extent.size() > 0)
    return metadata_.extent.at(0);

  return kUnknownEpisodeCount;
}

int Item::GetEpisodeLength() const {
  if (metadata_.extent.size() > 1)
    return metadata_.extent.at(1);

  return kUnknownEpisodeLength;
}

int Item::GetAiringStatus(bool check_date) const {
  if (!check_date)
    return metadata_.status;

  // TODO: Move
  if (IsFinishedAiring(*this))
    return kFinishedAiring;
  if (IsAiredYet(*this))
    return kAiring;

  return kNotYetAired;
}

const std::wstring& Item::GetTitle() const {
  return metadata_.title;
}

const std::wstring& Item::GetEnglishTitle(bool fallback) const {
  foreach_(it, metadata_.alternative)
    if (it->type == library::kTitleTypeLangEnglish)
      if (!it->value.empty())
        return it->value;

  if (fallback)
    return metadata_.title;

  return EmptyString();
}

std::vector<std::wstring> Item::GetSynonyms() const {
  std::vector<std::wstring> synonyms;

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

const std::wstring& Item::GetImageUrl() const {
  if (metadata_.resource.size() > 0)
    return metadata_.resource.at(0);

  return EmptyString();
}

enum_t Item::GetAgeRating() const {
  return metadata_.audience;
}

const std::vector<std::wstring>& Item::GetGenres() const {
  return metadata_.subject;
}

int Item::GetPopularity() const {
  if (metadata_.community.size() > 1)
    return ToInt(metadata_.community.at(1));

  return 0;
}

const std::vector<std::wstring>& Item::GetProducers() const {
  return metadata_.creator;
}

double Item::GetScore() const {
  if (metadata_.community.size() > 0)
    return ToDouble(metadata_.community.at(0));

  return 0.0;
}

const std::wstring& Item::GetSynopsis() const {
  return metadata_.description;
}

const time_t Item::GetLastModified() const {
  return metadata_.modified;
}

////////////////////////////////////////////////////////////////////////////////

void Item::SetId(const std::wstring& id, enum_t service) {
  if (metadata_.uid.size() < static_cast<size_t>(service) + 1)
    metadata_.uid.resize(service + 1);

  metadata_.uid.at(service) = id;
}

void Item::SetSlug(const std::wstring& slug) {
  if (metadata_.resource.size() < 2) {
    if (slug.empty())
      return;
    metadata_.resource.resize(2);
  }

  metadata_.resource.at(1) = slug;
}

void Item::SetSource(enum_t source) {
  metadata_.source = source;
}

void Item::SetType(int type) {
  metadata_.type = type;
}

void Item::SetEpisodeCount(int number) {
  if (metadata_.extent.size() < 1)
    metadata_.extent.resize(1);

  metadata_.extent.at(0) = number;

  // TODO: Call it separately
  if (number >= 0)
    if (static_cast<size_t>(number) > local_info_.available_episodes.size())
      local_info_.available_episodes.resize(number);
}

void Item::SetEpisodeLength(int number) {
  if (metadata_.extent.size() < 2) {
    if (number <= 0)
      return;
    metadata_.extent.resize(2);
  }

  metadata_.extent.at(1) = number;
}

void Item::SetAiringStatus(int status) {
  metadata_.status = status;
}

void Item::SetTitle(const std::wstring& title) {
  metadata_.title = title;
}

void Item::SetEnglishTitle(const std::wstring& title) {
  foreach_(it, metadata_.alternative) {
    if (it->type == library::kTitleTypeLangEnglish) {
      it->value = title;
      return;
    }
  }

  if (title.empty())
    return;

  library::Title new_title(library::kTitleTypeLangEnglish, title);

  metadata_.alternative.push_back(new_title);
}

void Item::InsertSynonym(const std::wstring& synonym) {
  if (synonym == GetTitle() || synonym == GetEnglishTitle())
    return;
  metadata_.alternative.push_back(
      library::Title(library::kTitleTypeSynonym, synonym));
}

void Item::SetSynonyms(const std::wstring& synonyms) {
  std::vector<std::wstring> temp;
  Split(synonyms, L"; ", temp);
  RemoveEmptyStrings(temp);

  SetSynonyms(temp);
}

void Item::SetSynonyms(const std::vector<std::wstring>& synonyms) {
  if (synonyms.empty() && metadata_.alternative.empty())
    return;

  auto iterator = std::remove_if(
      metadata_.alternative.begin(), metadata_.alternative.end(),
      [](const library::Title& title) {
        return title.type == library::kTitleTypeSynonym;
      });
  metadata_.alternative.erase(iterator, metadata_.alternative.end());

  for (const auto& synonym : synonyms) {
    InsertSynonym(synonym);
  }
}

void Item::SetDateStart(const Date& date) {
  if (metadata_.date.size() < 1) {
    if (!IsValidDate(date))
      return;
    metadata_.date.resize(1);
  }

  metadata_.date.at(0) = date;
}

void Item::SetDateEnd(const Date& date) {
  if (metadata_.date.size() < 2) {
    if (!IsValidDate(date))
      return;
    metadata_.date.resize(2);
  }

  metadata_.date.at(1) = date;
}

void Item::SetImageUrl(const std::wstring& url) {
  if (metadata_.resource.size() < 1) {
    if (url.empty())
      return;
    metadata_.resource.resize(1);
  }

  metadata_.resource.at(0) = url;
}

void Item::SetAgeRating(enum_t rating) {
  metadata_.audience = rating;
}

void Item::SetGenres(const std::wstring& genres) {
  std::vector<std::wstring> temp;
  Split(genres, L", ", temp);
  RemoveEmptyStrings(temp);

  SetGenres(temp);
}

void Item::SetGenres(const std::vector<std::wstring>& genres) {
  metadata_.subject = genres;
}

void Item::SetPopularity(int popularity) {
  if (metadata_.community.size() < 2) {
    if (popularity <= 0)
      return;
    metadata_.community.resize(2);
  }

  metadata_.community.at(1) = ToWstr(popularity);
}

void Item::SetProducers(const std::wstring& producers) {
  std::vector<std::wstring> temp;
  Split(producers, L", ", temp);
  RemoveEmptyStrings(temp);

  SetProducers(temp);
}

void Item::SetProducers(const std::vector<std::wstring>& producers) {
  metadata_.creator = producers;
}

void Item::SetScore(double score) {
  if (metadata_.community.size() < 1) {
    if (score <= 0.0)
      return;
    metadata_.community.resize(1);
  }

  metadata_.community.at(0) = score > 0.0 ? ToWstr(score) : L"";
}

void Item::SetSynopsis(const std::wstring& synopsis) {
  metadata_.description = synopsis;
}

void Item::SetLastModified(time_t modified) {
  metadata_.modified = modified;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int Item::GetMyLastWatchedEpisode(bool check_queue) const {
  if (!my_info_.get())
    return 0;

  HistoryItem* history_item = check_queue ?
      SearchHistory(kQueueSearchEpisode) : nullptr;

  return history_item ? *history_item->episode : my_info_->watched_episodes;
}

int Item::GetMyScore(bool check_queue) const {
  if (!my_info_.get())
    return 0;

  HistoryItem* history_item = check_queue ?
      SearchHistory(kQueueSearchScore) : nullptr;

  return history_item ? *history_item->score : my_info_->score;
}

int Item::GetMyStatus(bool check_queue) const {
  if (!my_info_.get())
    return kNotInList;

  HistoryItem* history_item = check_queue ?
      SearchHistory(kQueueSearchStatus) : nullptr;

  return history_item ? *history_item->status : my_info_->status;
}

int Item::GetMyRewatchedTimes(bool check_queue) const {
  if (!my_info_.get())
    return 0;

  HistoryItem* history_item = check_queue ?
      SearchHistory(kQueueSearchRewatchedTimes) : nullptr;

  return history_item ? *history_item->rewatched_times : my_info_->rewatched_times;
}

int Item::GetMyRewatching(bool check_queue) const {
  if (!my_info_.get())
    return FALSE;

  HistoryItem* history_item = check_queue ?
      SearchHistory(kQueueSearchRewatching) : nullptr;

  return history_item ? *history_item->enable_rewatching : my_info_->rewatching;
}

int Item::GetMyRewatchingEp() const {
  if (!my_info_.get())
    return 0;

  return my_info_->rewatching_ep;
}

const Date& Item::GetMyDateStart(bool check_queue) const {
  if (!my_info_.get())
    return EmptyDate();

  HistoryItem* history_item = check_queue ?
      SearchHistory(kQueueSearchDateStart) : nullptr;

  return history_item ? *history_item->date_start : my_info_->date_start;
}

const Date& Item::GetMyDateEnd(bool check_queue) const {
  if (!my_info_.get())
    return EmptyDate();

  HistoryItem* history_item = check_queue ?
      SearchHistory(kQueueSearchDateEnd) : nullptr;

  return history_item ? *history_item->date_finish : my_info_->date_finish;
}

const std::wstring& Item::GetMyLastUpdated() const {
  if (!my_info_.get())
    return EmptyString();

  return my_info_->last_updated;
}

const std::wstring& Item::GetMyTags(bool check_queue) const {
  if (!my_info_.get())
    return EmptyString();

  HistoryItem* history_item = check_queue ?
    SearchHistory(kQueueSearchTags) : nullptr;

  return history_item ? *history_item->tags : my_info_->tags;
}

////////////////////////////////////////////////////////////////////////////////

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

void Item::SetMyRewatchedTimes(int rewatched_times) {
  assert(my_info_.get());

  my_info_->rewatched_times = rewatched_times;
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

void Item::SetMyLastUpdated(const std::wstring& last_updated) {
  assert(my_info_.get());

  my_info_->last_updated = last_updated;
}

void Item::SetMyTags(const std::wstring& tags) {
  assert(my_info_.get());

  my_info_->tags = tags;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int Item::GetAvailableEpisodeCount() const {
  return static_cast<int>(local_info_.available_episodes.size());
}

const std::wstring& Item::GetFolder() const {
  return local_info_.folder;
}

int Item::GetLastAiredEpisodeNumber(bool estimate) const {
  if (local_info_.last_aired_episode)
    return local_info_.last_aired_episode;

  // No need to estimate if the series isn't currently airing
  switch (GetAiringStatus()) {
    case kFinishedAiring:
      return GetEpisodeCount();
    case kNotYetAired:
    case kUnknownStatus:
      return 0;
  }

  if (estimate)
    return EstimateLastAiredEpisodeNumber(*this);

  return local_info_.last_aired_episode;
}

const std::wstring& Item::GetNextEpisodePath() const {
  return local_info_.next_episode_path;
}

bool Item::GetPlaying() const {
  return local_info_.playing;
}

bool Item::GetUseAlternative() const {
  return local_info_.use_alternative;
}

const std::vector<std::wstring>& Item::GetUserSynonyms() const {
  return local_info_.synonyms;
}

////////////////////////////////////////////////////////////////////////////////

bool Item::SetEpisodeAvailability(int number, bool available,
                                  const std::wstring& path) {
  if (number == 0)
    number = 1;

  if (number <= GetEpisodeCount() || !IsValidEpisodeCount(GetEpisodeCount())) {
    if (static_cast<size_t>(number) > local_info_.available_episodes.size()) {
      local_info_.available_episodes.resize(number);
    }
    local_info_.available_episodes.at(number - 1) = available;
    if (number == GetMyLastWatchedEpisode() + 1) {
      SetNextEpisodePath(path);
    }

    ui::OnLibraryEntryChange(GetId());

    return true;
  }

  return false;
}

void Item::SetFolder(const std::wstring& folder) {
  local_info_.folder = folder;
}

void Item::SetLastAiredEpisodeNumber(int number) {
  if (number > local_info_.last_aired_episode) {
    if (GetAiringStatus() == kFinishedAiring) {
      local_info_.last_aired_episode = GetEpisodeCount();
    } else {
      local_info_.last_aired_episode = number;
    }
  }
}

void Item::SetNextEpisodePath(const std::wstring& path) {
  local_info_.next_episode_path = path;
}

void Item::SetPlaying(bool playing) {
  local_info_.playing = playing;
}

void Item::SetUseAlternative(bool use_alternative) {
  local_info_.use_alternative = use_alternative;
}

void Item::SetUserSynonyms(const std::wstring& synonyms) {
  std::vector<std::wstring> temp;
  Split(synonyms, L"; ", temp);

  SetUserSynonyms(temp);
}

void Item::SetUserSynonyms(const std::vector<std::wstring>& synonyms) {
  local_info_.synonyms = synonyms;
  RemoveEmptyStrings(local_info_.synonyms);

  if (!synonyms.empty() && CurrentEpisode.anime_id == anime::ID_NOTINLIST) {
    CurrentEpisode.Set(anime::ID_UNKNOWN);
  }
}

////////////////////////////////////////////////////////////////////////////////

bool Item::IsEpisodeAvailable(int number) const {
  if (number < 1)
    number = 1;
  if (static_cast<size_t>(number) > local_info_.available_episodes.size())
    return false;

  return local_info_.available_episodes.at(number - 1);
}

bool Item::IsNextEpisodeAvailable() const {
  return IsEpisodeAvailable(GetMyLastWatchedEpisode() + 1);
}

bool Item::UserSynonymsAvailable() const {
  return !local_info_.synonyms.empty();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

HistoryItem* Item::SearchHistory(int search_mode) const {
  return History.queue.FindItem(GetId(), search_mode);
}

}  // namespace anime