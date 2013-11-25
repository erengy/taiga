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

#ifndef ANIME_ITEM_H
#define ANIME_ITEM_H

#include "base/std.h"

#include "anime.h"

namespace anime {
class Database;
class Episode;
class Item;
}
class Date;
class EventItem;

namespace anime {

class Item {
 public:
  Item();
  virtual ~Item();

  // Accessors for series information
  int GetId() const;
  int GetType() const;
  int GetEpisodeCount(bool estimation = false) const;
  int GetAiringStatus(bool check_date = true) const;
  const wstring& GetTitle() const;
  const wstring& GetEnglishTitle(bool fallback = false) const;
  const vector<wstring>& GetSynonyms() const;
  const Date& GetDate(DateType type) const;
  const wstring& GetImageUrl() const;
  const wstring& GetGenres() const;
  const wstring& GetPopularity() const;
  const wstring& GetProducers() const;
  const wstring& GetRank() const;
  const wstring& GetScore() const;
  const wstring& GetSynopsis() const;

  // Accessors for user information - if check_events flag is enabled, the
  // latest relevant value in event queue is returned.
  int GetMyLastWatchedEpisode(bool check_events = true) const;
  int GetMyScore(bool check_events = true) const;
  int GetMyStatus(bool check_events = true) const;
  int GetMyRewatching(bool check_events = true) const;
  int GetMyRewatchingEp() const;
  const Date GetMyDate(DateType type, bool check_events = true) const;
  const wstring GetMyLastUpdated() const;
  const wstring GetMyTags(bool check_events = true) const;

  // Mutators for series information
  void SetId(int anime_id);
  void SetType(int type);
  void SetEpisodeCount(int number);
  void SetAiringStatus(int status);
  void SetTitle(const wstring& title);
  void SetEnglishTitle(const wstring& title);
  void SetSynonyms(const wstring& synonyms);
  void SetSynonyms(const vector<wstring>& synonyms);
  void SetDate(DateType type, const Date& date);
  void SetImageUrl(const wstring& url);
  void SetGenres(const wstring& genres);
  void SetPopularity(const wstring& popularity);
  void SetProducers(const wstring& producers);
  void SetRank(const wstring& rank);
  void SetScore(const wstring& score);
  void SetSynopsis(const wstring& synopsis);
  
  // Mutators for user information
  void SetMyLastWatchedEpisode(int number);
  void SetMyScore(int score);
  void SetMyStatus(int status);
  void SetMyRewatching(int rewatching);
  void SetMyRewatchingEp(int rewatching_ep);
  void SetMyDate(DateType type, const Date& date);
  void SetMyLastUpdated(const wstring& last_updated);
  void SetMyTags(const wstring& tags);
  
  // Functions related to airing status
  bool IsAiredYet() const;
  bool IsFinishedAiring() const;
  
  // Functions related to episode availability
  bool CheckEpisodes(int number = -1, bool check_folder = false);
  int GetAvailableEpisodeCount() const;
  int GetLastAiredEpisodeNumber(bool estimate = false);
  wstring GetNewEpisodePath() const;
  bool IsEpisodeAvailable(int number) const;
  bool IsNewEpisodeAvailable() const;
  bool PlayEpisode(int number);
  bool SetEpisodeAvailability(int number, bool available, const wstring& path);
  void SetLastAiredEpisodeNumber(int number);
  void SetNewEpisodePath(const wstring& path);

  // For anime-specific folders on user's computer
  bool CheckFolder();
  const wstring GetFolder() const;
  void SetFolder(const wstring& folder);
  
  // More than one anime may have their playing flag on.
  bool GetPlaying() const;
  void SetPlaying(bool playing);
  
  // For alternative titles provided by user
  const vector<wstring>& GetUserSynonyms() const;
  void SetUserSynonyms(const wstring& synonyms);
  void SetUserSynonyms(const vector<wstring>& synonyms);
  bool UserSynonymsAvailable() const;
  bool GetUseAlternative() const;
  void SetUseAlternative(bool use_alternative);
  
  // A database item may not be in user's list.
  void AddtoUserList();
  bool IsInList() const;
  void RemoveFromUserList();

  // Is this item old enough to be updated? See last_modified for more
  // information.
  bool IsOldEnough() const;

  // After a successful update, an event item is removed from the queue and the
  // relevant anime item is edited.
  void Edit(const EventItem& item);

  // Following functions are called when a new episode is recognized. Actual
  // time depends on user settings.
  void StartWatching(Episode& episode);
  void EndWatching(Episode episode);
  bool IsUpdateAllowed(const Episode& episode, bool ignore_update_time);
  void UpdateList(Episode& episode);
  void AddToQueue(const Episode& episode, bool change_status);

  // MAL's API doesn't provide searching anime by ID, and some titles return
  // no result due to special characters. Season data provide safe-to-search
  // titles with keep_title flag on.
  // TODO: Remove after searching by ID is made possible.
  bool keep_title;
  
  // An item's series information will only be updated only if last_modified
  // value is significantly older than the new one's. This helps us lower
  // the number of requests we send to MAL.
  time_t last_modified;

 private:
  // Helper function
  EventItem* SearchHistory(int search_mode) const;

  // Series information, stored in db\anime.xml
  SeriesInformation series_info_;

  // User information, stored in user\<username>\anime.xml - some items are not
  // in user's list, thus this member is not valid for every item.
  std::shared_ptr<MyInformation> my_info_;

  // Local information, stored temporarily
  LocalInformation local_info_;

  // Pointer to the parent database which holds this item
  static Database* database_;
};

} // namespace anime

#endif // ANIME_ITEM_H