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

#include <assert.h>

#include "base/string.h"
#include "base/time.h"
#include "media/anime_item.h"
#include "media/anime_util.h"
#include "media/library/queue.h"
#include "sync/service.h"
#include "ui/ui.h"

namespace anime {

Item::Item() {
  series_.uids.resize(sync::kLastService + 1);
}

Item::~Item() {
}

////////////////////////////////////////////////////////////////////////////////

int Item::GetId() const {
  return series_.id;
}

const std::wstring& Item::GetId(enum_t service) const {
  assert(series_.uids.size() > service);

  return series_.uids.at(service);
}

const std::wstring& Item::GetSlug() const {
  return series_.slug;
}

enum_t Item::GetSource() const {
  return series_.source;
}

int Item::GetType() const {
  return series_.type;
}

int Item::GetEpisodeCount() const {
  return series_.episode_count;
}

int Item::GetEpisodeLength() const {
  return series_.episode_length;
}

int Item::GetAiringStatus(bool check_date) const {
  if (!check_date)
    return series_.status;

  return anime::GetAiringStatus(*this);
}

const std::wstring& Item::GetTitle() const {
  return series_.titles.romaji;
}

const std::wstring& Item::GetEnglishTitle(bool fallback) const {
  if (series_.titles.english.empty() && fallback)
    return series_.titles.romaji;

  return series_.titles.english;
}

const std::wstring& Item::GetJapaneseTitle() const {
  return series_.titles.japanese;
}

std::vector<std::wstring> Item::GetSynonyms() const {
  return series_.titles.synonyms;
}

const Date& Item::GetDateStart() const {
  return series_.start_date;
}

const Date& Item::GetDateEnd() const {
  return series_.end_date;
}

const std::wstring& Item::GetImageUrl() const {
  return series_.image_url;
}

enum_t Item::GetAgeRating() const {
  return series_.age_rating;
}

const std::vector<std::wstring>& Item::GetGenres() const {
  return series_.genres;
}

int Item::GetPopularity() const {
  return series_.popularity_rank;
}

const std::vector<std::wstring>& Item::GetProducers() const {
  return series_.producers;
}

double Item::GetScore() const {
  return series_.score;
}

const std::wstring& Item::GetSynopsis() const {
  return series_.synopsis;
}

const time_t Item::GetLastModified() const {
  return series_.last_modified;
}

////////////////////////////////////////////////////////////////////////////////

void Item::SetId(const std::wstring& id, enum_t service) {
  if (series_.uids.size() < static_cast<size_t>(service) + 1)
    series_.uids.resize(service + 1);

  series_.uids.at(service) = id;

  if (service == 0) {
    series_.id = ToInt(id);
  }
}

void Item::SetSlug(const std::wstring& slug) {
  series_.slug = slug;
}

void Item::SetSource(enum_t source) {
  series_.source = source;
}

void Item::SetType(int type) {
  series_.type = static_cast<SeriesType>(type);
}

void Item::SetEpisodeCount(int number) {
  series_.episode_count = number;

  // TODO: Call it separately
  if (number >= 0)
    if (static_cast<size_t>(number) > local_info_.available_episodes.size()) {
      local_info_.available_episodes.resize(number);
      local_info_.episode_paths.resize(number);
    }
}

void Item::SetEpisodeLength(int number) {
  series_.episode_length = number;
}

void Item::SetAiringStatus(int status) {
  series_.status = static_cast<SeriesStatus>(status);
}

void Item::SetTitle(const std::wstring& title) {
  series_.titles.romaji = title;
}

void Item::SetEnglishTitle(const std::wstring& title) {
  series_.titles.english = title;
}

void Item::SetJapaneseTitle(const std::wstring& title) {
  series_.titles.japanese = title;
}

void Item::InsertSynonym(const std::wstring& synonym) {
  if (synonym.empty() || synonym == GetTitle() ||
      synonym == GetEnglishTitle() || synonym == GetJapaneseTitle())
    return;
  series_.titles.synonyms.push_back(synonym);
}

void Item::SetSynonyms(const std::wstring& synonyms) {
  std::vector<std::wstring> temp;
  Split(synonyms, L"; ", temp);
  RemoveEmptyStrings(temp);

  SetSynonyms(temp);
}

void Item::SetSynonyms(const std::vector<std::wstring>& synonyms) {
  if (synonyms.empty() && series_.titles.synonyms.empty())
    return;

  series_.titles.synonyms.clear();

  for (const auto& synonym : synonyms) {
    InsertSynonym(synonym);
  }
}

void Item::SetDateStart(const Date& date) {
  series_.start_date = date;
}

void Item::SetDateStart(const std::wstring& date) {
  SetDateStart(Date(date));
}

void Item::SetDateEnd(const Date& date) {
  series_.end_date = date;
}

void Item::SetDateEnd(const std::wstring& date) {
  SetDateEnd(Date(date));
}

void Item::SetImageUrl(const std::wstring& url) {
  series_.image_url = url;
}

void Item::SetAgeRating(enum_t rating) {
  series_.age_rating = static_cast<AgeRating>(rating);
}

void Item::SetGenres(const std::wstring& genres) {
  std::vector<std::wstring> temp;
  Split(genres, L", ", temp);
  RemoveEmptyStrings(temp);

  SetGenres(temp);
}

void Item::SetGenres(const std::vector<std::wstring>& genres) {
  series_.genres = genres;
}

void Item::SetPopularity(int popularity) {
  series_.popularity_rank = popularity;
}

void Item::SetProducers(const std::wstring& producers) {
  std::vector<std::wstring> temp;
  Split(producers, L", ", temp);
  RemoveEmptyStrings(temp);

  SetProducers(temp);
}

void Item::SetProducers(const std::vector<std::wstring>& producers) {
  series_.producers = producers;
}

void Item::SetScore(double score) {
  series_.score = score > 0.0 ? static_cast<float>(score) : 0.0f;
}

void Item::SetSynopsis(const std::wstring& synopsis) {
  series_.synopsis = synopsis;
}

void Item::SetLastModified(time_t modified) {
  series_.last_modified = modified;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const std::wstring& Item::GetMyId() const {
  if (!my_info_.get())
    return EmptyString();

  return my_info_->id;
}

int Item::GetMyLastWatchedEpisode(bool check_queue) const {
  if (!my_info_.get())
    return 0;

  library::QueueItem* queue_item = check_queue ?
      SearchQueue(library::QueueSearch::Episode) : nullptr;

  return queue_item ? *queue_item->episode : my_info_->watched_episodes;
}

int Item::GetMyScore(bool check_queue) const {
  if (!my_info_.get())
    return 0;

  library::QueueItem* queue_item = check_queue ?
      SearchQueue(library::QueueSearch::Score) : nullptr;

  return queue_item ? *queue_item->score : my_info_->score;
}

int Item::GetMyStatus(bool check_queue) const {
  if (!my_info_.get())
    return kNotInList;

  library::QueueItem* queue_item = check_queue ?
      SearchQueue(library::QueueSearch::Status) : nullptr;

  return queue_item ? *queue_item->status : my_info_->status;
}

int Item::GetMyRewatchedTimes(bool check_queue) const {
  if (!my_info_.get())
    return 0;

  library::QueueItem* queue_item = check_queue ?
      SearchQueue(library::QueueSearch::RewatchedTimes) : nullptr;

  return queue_item ? *queue_item->rewatched_times : my_info_->rewatched_times;
}

bool Item::GetMyRewatching(bool check_queue) const {
  if (!my_info_.get())
    return false;

  library::QueueItem* queue_item = check_queue ?
      SearchQueue(library::QueueSearch::Rewatching) : nullptr;

  return queue_item ? *queue_item->enable_rewatching : my_info_->rewatching;
}

int Item::GetMyRewatchingEp() const {
  if (!my_info_.get())
    return 0;

  return my_info_->rewatching_ep;
}

const Date& Item::GetMyDateStart(bool check_queue) const {
  if (!my_info_.get())
    return EmptyDate();

  library::QueueItem* queue_item = check_queue ?
      SearchQueue(library::QueueSearch::DateStart) : nullptr;

  return queue_item ? *queue_item->date_start : my_info_->date_start;
}

const Date& Item::GetMyDateEnd(bool check_queue) const {
  if (!my_info_.get())
    return EmptyDate();

  library::QueueItem* queue_item = check_queue ?
      SearchQueue(library::QueueSearch::DateEnd) : nullptr;

  return queue_item ? *queue_item->date_finish : my_info_->date_finish;
}

const std::wstring& Item::GetMyLastUpdated() const {
  if (!my_info_.get())
    return EmptyString();

  return my_info_->last_updated;
}

const std::wstring& Item::GetMyTags(bool check_queue) const {
  if (!my_info_.get())
    return EmptyString();

  library::QueueItem* queue_item = check_queue ?
      SearchQueue(library::QueueSearch::Tags) : nullptr;

  return queue_item ? *queue_item->tags : my_info_->tags;
}

const std::wstring& Item::GetMyNotes(bool check_queue) const {
  if (!my_info_.get())
    return EmptyString();

  library::QueueItem* queue_item = check_queue ?
      SearchQueue(library::QueueSearch::Notes) : nullptr;

  return queue_item ? *queue_item->notes : my_info_->notes;
}

////////////////////////////////////////////////////////////////////////////////

void Item::SetMyId(const std::wstring& id) {
  assert(my_info_.get());

  my_info_->id = id;
}

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

void Item::SetMyRewatching(bool rewatching) {
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

void Item::SetMyDateStart(const std::wstring& date) {
  SetMyDateStart(Date(date));
}

void Item::SetMyDateEnd(const Date& date) {
  assert(my_info_.get());

  my_info_->date_finish = date;
}

void Item::SetMyDateEnd(const std::wstring& date) {
  SetMyDateEnd(Date(date));
}

void Item::SetMyLastUpdated(const std::wstring& last_updated) {
  assert(my_info_.get());

  my_info_->last_updated = last_updated;
}

void Item::SetMyTags(const std::wstring& tags) {
  assert(my_info_.get());

  my_info_->tags = tags;
}

void Item::SetMyNotes(const std::wstring& notes) {
  assert(my_info_.get());

  my_info_->notes = notes;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int Item::GetAvailableEpisodeCount() const {
  return static_cast<int>(local_info_.available_episodes.size());
}

const std::wstring& Item::GetFolder() const {
  return local_info_.folder;
}

int Item::GetLastAiredEpisodeNumber() const {
  return local_info_.last_aired_episode;
}

const std::wstring& Item::GetNextEpisodePath() const {
  return local_info_.next_episode_path;
}

const std::wstring& Item::GetEpisodePath(int number) const {
  return local_info_.episode_paths.at(number - 1);
}

time_t Item::GetNextEpisodeTime() const {
  return local_info_.next_episode_time;
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
      local_info_.episode_paths.resize(number);
    }
    local_info_.available_episodes.at(number - 1) = available;
    local_info_.episode_paths.at(number - 1) = path;
    if (number == GetMyLastWatchedEpisode() + 1) {
      SetNextEpisodePath(path);
    }

    ui::OnEpisodeAvailabilityChange(GetId());

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

void Item::SetNextEpisodeTime(const time_t time) {
  local_info_.next_episode_time = time;
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

library::QueueItem* Item::SearchQueue(library::QueueSearch search_mode) const {
  return library::queue.FindItem(GetId(), search_mode);
}

}  // namespace anime
