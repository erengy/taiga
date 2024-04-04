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

#include <algorithm>
#include <assert.h>

#include "media/anime_item.h"

#include "base/string.h"
#include "base/time.h"
#include "media/anime_util.h"
#include "media/library/queue.h"
#include "sync/service.h"
#include "taiga/settings.h"
#include "ui/ui.h"

namespace anime {

int Item::GetId() const {
  return series_.id;
}

const std::wstring& Item::GetId(sync::ServiceId service) const {
  const auto it = series_.uids.find(service);
  return it != series_.uids.end() ? it->second : EmptyString();
}

const std::wstring& Item::GetSlug() const {
  return series_.slug;
}

sync::ServiceId Item::GetSource() const {
  return series_.source;
}

SeriesType Item::GetType() const {
  return series_.type;
}

int Item::GetEpisodeCount() const {
  return series_.episode_count;
}

int Item::GetEpisodeLength() const {
  return series_.episode_length;
}

SeriesStatus Item::GetAiringStatus(bool check_date) const {
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

AgeRating Item::GetAgeRating() const {
  return series_.age_rating;
}

const std::vector<std::wstring>& Item::GetGenres() const {
  return series_.genres;
}

const std::vector<std::wstring>& Item::GetTags() const {
  return series_.tags;
}

int Item::GetPopularity() const {
  return series_.popularity_rank;
}

const std::vector<std::wstring>& Item::GetProducers() const {
  return series_.producers;
}

const std::vector<std::wstring>& Item::GetStudios() const {
  return series_.studios;
}

double Item::GetScore() const {
  return series_.score;
}

const std::wstring& Item::GetSynopsis() const {
  return series_.synopsis;
}

const std::wstring& Item::GetTrailerId() const {
  return series_.trailer_id;
}

const time_t Item::GetLastModified() const {
  return series_.last_modified;
}

int Item::GetLastAiredEpisodeNumber() const {
  return series_.last_aired_episode;
}

time_t Item::GetNextEpisodeTime() const {
  return series_.next_episode_time;
}

////////////////////////////////////////////////////////////////////////////////

void Item::SetId(const std::wstring& id, sync::ServiceId service) {
  series_.uids[service] = id;

  if (service == sync::GetCurrentServiceId()) {
    series_.id = ToInt(id);
  }
}

void Item::SetSlug(const std::wstring& slug) {
  series_.slug = slug;
}

void Item::SetSource(sync::ServiceId source) {
  series_.source = source;
}

void Item::SetType(SeriesType type) {
  series_.type = type;
}

void Item::SetEpisodeCount(int number) {
  series_.episode_count = std::clamp(number, 0, kMaxEpisodeCount);

  // TODO: Call it separately
  if (number >= 0)
    if (static_cast<size_t>(number) > local_info_.available_episodes.size())
      local_info_.available_episodes.resize(number);
}

void Item::SetEpisodeLength(int number) {
  series_.episode_length = number;
}

void Item::SetAiringStatus(SeriesStatus status) {
  series_.status = status;
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

void Item::SetAgeRating(AgeRating rating) {
  series_.age_rating = rating;
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

void Item::SetTags(const std::wstring& tags) {
  std::vector<std::wstring> temp;
  Split(tags, L", ", temp);
  RemoveEmptyStrings(temp);

  SetTags(temp);
}

void Item::SetTags(const std::vector<std::wstring>& tags) {
  series_.tags = tags;
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

void Item::SetStudios(const std::wstring& studios) {
  std::vector<std::wstring> temp;
  Split(studios, L", ", temp);
  RemoveEmptyStrings(temp);

  SetStudios(temp);
}

void Item::SetStudios(const std::vector<std::wstring>& studios) {
  series_.studios = studios;
}

void Item::SetScore(double score) {
  series_.score = score > 0.0 ? static_cast<float>(score) : 0.0f;
}

void Item::SetSynopsis(const std::wstring& synopsis) {
  series_.synopsis = synopsis;
}

void Item::SetTrailerId(const std::wstring& trailer_id) {
  series_.trailer_id = trailer_id;
}

void Item::SetLastModified(time_t modified) {
  series_.last_modified = modified;
}

void Item::SetLastAiredEpisodeNumber(int number) {
  if (number > series_.last_aired_episode) {
    series_.last_aired_episode = number;
  }
}

void Item::SetNextEpisodeTime(const time_t time) {
  series_.next_episode_time = time;
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

MyStatus Item::GetMyStatus(bool check_queue) const {
  if (!my_info_.get())
    return MyStatus::NotInList;

  library::QueueItem* queue_item = check_queue ?
      SearchQueue(library::QueueSearch::Status) : nullptr;

  return queue_item ? *queue_item->status : my_info_->status;
}

bool Item::GetMyPrivate() const {
  if (!my_info_.get())
    return false;

  return my_info_->is_private;
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

  my_info_->watched_episodes = std::clamp(number, 0, kMaxEpisodeCount);
}

void Item::SetMyScore(int score) {
  assert(my_info_.get());

  my_info_->score = score;
}

void Item::SetMyStatus(MyStatus status) {
  assert(my_info_.get());

  my_info_->status = status;
}

void Item::SetMyPrivate(bool is_private) {
  assert(my_info_.get());

  my_info_->is_private = is_private;
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

void Item::SetMyNotes(const std::wstring& notes) {
  assert(my_info_.get());

  my_info_->notes = notes;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int Item::GetAvailableEpisodeCount() const {
  return static_cast<int>(local_info_.available_episodes.size());
}

std::wstring Item::GetFolder() const {
  return taiga::settings.GetAnimeFolder(GetId());
}

const std::wstring& Item::GetNextEpisodePath() const {
  return local_info_.next_episode_path;
}

bool Item::GetUseAlternative() const {
  return taiga::settings.GetAnimeUseAlternative(GetId());
}

std::vector<std::wstring> Item::GetUserSynonyms() const {
  return taiga::settings.GetAnimeUserSynonyms(GetId());
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

    ui::OnEpisodeAvailabilityChange(GetId());

    return true;
  }

  return false;
}

void Item::SetFolder(const std::wstring& folder) {
  taiga::settings.SetAnimeFolder(GetId(), folder);
}

void Item::SetNextEpisodePath(const std::wstring& path) {
  local_info_.next_episode_path = path;
}

void Item::SetUseAlternative(bool use_alternative) {
  taiga::settings.SetAnimeUseAlternative(GetId(), use_alternative);
}

void Item::SetUserSynonyms(const std::wstring& synonyms) {
  std::vector<std::wstring> temp;
  Split(synonyms, L"; ", temp);

  SetUserSynonyms(temp);
}

void Item::SetUserSynonyms(std::vector<std::wstring> synonyms) {
  RemoveEmptyStrings(synonyms);

  taiga::settings.SetAnimeUserSynonyms(GetId(), synonyms);

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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void Item::AddtoUserList() {
  if (!my_info_.get()) {
    my_info_.reset(new MyInformation);
  }
}

bool Item::IsInList() const {
  return my_info_.get() && GetMyStatus() != MyStatus::NotInList;
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
