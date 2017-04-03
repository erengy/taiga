/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#pragma once

#include <string>
#include <vector>
#include <memory>

#include "anime.h"
#include "metadata.h"

namespace anime {
class Database;
class Episode;
class Item;
}
class Date;
class HistoryItem;

namespace anime {

class Item {
public:
  Item();
  virtual ~Item();

  //////////////////////////////////////////////////////////////////////////////
  // Metadata

  int GetId() const;
  const std::wstring& GetId(enum_t service) const;
  const std::wstring& GetSlug() const;
  enum_t GetSource() const;
  int GetType() const;
  int GetEpisodeCount() const;
  int GetEpisodeLength() const;
  int GetAiringStatus(bool check_date = true) const;
  const std::wstring& GetTitle() const;
  const std::wstring& GetEnglishTitle(bool fallback = false) const;
  const std::wstring& GetJapaneseTitle() const;
  std::vector<std::wstring> GetSynonyms() const;
  const Date& GetDateStart() const;
  const Date& GetDateEnd() const;
  const std::wstring& GetImageUrl() const;
  enum_t GetAgeRating() const;
  const std::vector<std::wstring>& GetGenres() const;
  int GetPopularity() const;
  const std::vector<std::wstring>& GetProducers() const;
  double GetScore() const;
  const std::wstring& GetSynopsis() const;
  const time_t GetLastModified() const;

  void SetId(const std::wstring& id, enum_t service);
  void SetSlug(const std::wstring& slug);
  void SetSource(enum_t source);
  void SetType(int type);
  void SetEpisodeCount(int number);
  void SetEpisodeLength(int number);
  void SetAiringStatus(int status);
  void SetTitle(const std::wstring& title);
  void SetEnglishTitle(const std::wstring& title);
  void SetJapaneseTitle(const std::wstring& title);
  void InsertSynonym(const std::wstring& synonym);
  void SetSynonyms(const std::wstring& synonyms);
  void SetSynonyms(const std::vector<std::wstring>& synonyms);
  void SetDateStart(const Date& date);
  void SetDateStart(const std::wstring& date);
  void SetDateEnd(const Date& date);
  void SetDateEnd(const std::wstring& date);
  void SetImageUrl(const std::wstring& url);
  void SetAgeRating(enum_t rating);
  void SetGenres(const std::wstring& genres);
  void SetGenres(const std::vector<std::wstring>& genres);
  void SetPopularity(int popularity);
  void SetProducers(const std::wstring& producers);
  void SetProducers(const std::vector<std::wstring>& producers);
  void SetScore(double score);
  void SetSynopsis(const std::wstring& synopsis);
  void SetLastModified(time_t modified);

  //////////////////////////////////////////////////////////////////////////////
  // Library data

  const std::wstring& GetMyId() const;
  int GetMyLastWatchedEpisode(bool check_queue = true) const;
  int GetMyScore(bool check_queue = true) const;
  int GetMyStatus(bool check_queue = true) const;
  int GetMyRewatchedTimes(bool check_queue = true) const;
  int GetMyRewatching(bool check_queue = true) const;
  int GetMyRewatchingEp() const;
  const Date& GetMyDateStart(bool check_queue = true) const;
  const Date& GetMyDateEnd(bool check_queue = true) const;
  const std::wstring& GetMyLastUpdated() const;
  const std::wstring& GetMyTags(bool check_queue = true) const;
  const std::wstring& GetMyNotes(bool check_queue = true) const;

  void SetMyId(const std::wstring& id);
  void SetMyLastWatchedEpisode(int number);
  void SetMyScore(int score);
  void SetMyStatus(int status);
  void SetMyRewatchedTimes(int rewatched_times);
  void SetMyRewatching(int rewatching);
  void SetMyRewatchingEp(int rewatching_ep);
  void SetMyDateStart(const Date& date);
  void SetMyDateStart(const std::wstring& date);
  void SetMyDateEnd(const Date& date);
  void SetMyDateEnd(const std::wstring& date);
  void SetMyLastUpdated(const std::wstring& last_updated);
  void SetMyTags(const std::wstring& tags);
  void SetMyNotes(const std::wstring& notes);

  //////////////////////////////////////////////////////////////////////////////
  // Local data

  int GetAvailableEpisodeCount() const;
  const std::wstring& GetFolder() const;
  int GetLastAiredEpisodeNumber(bool estimate = false) const;
  const std::wstring& GetNextEpisodePath() const;
  bool GetPlaying() const;
  bool GetUseAlternative() const;
  const std::vector<std::wstring>& GetUserSynonyms() const;

  bool SetEpisodeAvailability(int number, bool available, const std::wstring& path);
  void SetFolder(const std::wstring& folder);
  void SetLastAiredEpisodeNumber(int number);
  void SetNextEpisodePath(const std::wstring& path);
  void SetPlaying(bool playing);
  void SetUseAlternative(bool use_alternative);
  void SetUserSynonyms(const std::wstring& synonyms);
  void SetUserSynonyms(const std::vector<std::wstring>& synonyms);

  bool IsEpisodeAvailable(int number) const;
  bool IsNextEpisodeAvailable() const;
  bool UserSynonymsAvailable() const;

  //////////////////////////////////////////////////////////////////////////////

  // A database item may not be in user's list.
  void AddtoUserList();
  bool IsInList() const;
  void RemoveFromUserList();

private:
  // Helper function
  HistoryItem* SearchHistory(int search_mode) const;

  // Series information, stored in db\anime.xml
  library::Metadata metadata_;

  // User information, stored in user\<username>\anime.xml - some items are not
  // in user's list, thus this member is not valid for every item.
  std::shared_ptr<MyInformation> my_info_;

  // Local information, stored temporarily
  LocalInformation local_info_;

  // Pointer to the parent database which holds this item
  static Database* database_;
};

}  // namespace anime
