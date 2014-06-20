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

#ifndef TAIGA_LIBRARY_ANIME_UTIL_H
#define TAIGA_LIBRARY_ANIME_UTIL_H

#include <string>
#include <vector>

#include "base/comparable.h"

class Date;

namespace anime {

class Episode;
class Item;

bool IsAiredYet(const Item& item);
bool IsFinishedAiring(const Item& item);
int EstimateLastAiredEpisodeNumber(const Item& item);

bool IsItemOldEnough(const Item& item);
bool MetadataNeedsRefresh(const Item& item);

bool IsNsfw(const Item& item);

bool PlayEpisode(int anime_id, int number);
bool PlayLastEpisode(int anime_id);
bool PlayNextEpisode(int anime_id);
bool PlayRandomAnime();
bool PlayRandomEpisode(Item& item);
bool LinkEpisodeToAnime(Episode& episode, int anime_id);
void StartWatching(Item& item, Episode& episode);
void EndWatching(Item& item, Episode episode);
bool IsUpdateAllowed(Item& item, const Episode& episode, bool ignore_update_time);
void UpdateList(Item& item, Episode& episode);
void AddToQueue(Item& item, const Episode& episode, bool change_status);

bool GetFansubFilter(int anime_id, std::vector<std::wstring>& groups);
bool SetFansubFilter(int anime_id, const std::wstring& group_name);

std::wstring GetImagePath(int anime_id = -1);

void GetUpcomingTitles(std::vector<int>& anime_ids);

bool IsInsideRootFolders(const std::wstring& path);

int GetEpisodeHigh(const std::wstring& episode_number);
int GetEpisodeLow(const std::wstring& episode_number);
bool IsAllEpisodesAvailable(const Item& item);
bool IsEpisodeRange(const std::wstring& episode_number);
bool IsValidEpisode(int episode, int total);
bool IsValidEpisode(int episode, int watched, int total);
std::wstring JoinEpisodeNumbers(const std::vector<int>& input);
void SplitEpisodeNumbers(const std::wstring& input, std::vector<int>& output);
int EstimateEpisodeCount(const Item& item);
void ChangeEpisode(int anime_id, int value);
void DecrementEpisode(int anime_id);
void IncrementEpisode(int anime_id);

std::wstring TranslateMyStatus(int value, bool add_count);
std::wstring TranslateNumber(int value, const std::wstring& default_char = L"-");
std::wstring TranslateScore(int value, const std::wstring& default_char = L"-");
std::wstring TranslateScoreFull(int value);
std::wstring TranslateStatus(int value);
std::wstring TranslateType(int value);

int TranslateResolution(const std::wstring& str, bool return_validity = false);

////////////////////////////////////////////////////////////////////////////////

class Season : public base::Comparable<Season> {
public:
  Season();
  ~Season() {}

  Season& operator = (const Season& season);

  enum Name {
    kUnknown,
    kWinter,
    kSpring,
    kSummer,
    kFall
  };

  Name name;
  unsigned short year;

private:
  base::CompareResult Compare(const Season& season) const;
};

bool IsValidDate(const Date& date);
void GetSeasonInterval(const std::wstring& season, Date& date_start, Date& date_end);
std::wstring TranslateDate(const Date& date);
Season TranslateDateToSeason(const Date& date);
std::wstring TranslateDateToSeasonString(const Date& date);
std::wstring TranslateSeasonToMonths(const std::wstring& season);

}  // namespace anime

#endif  // TAIGA_LIBRARY_ANIME_UTIL_H