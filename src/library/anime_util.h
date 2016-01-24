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

#include "library/anime_episode.h"

class Date;

namespace anime {

class Item;
class Season;

bool IsValidId(int anime_id);

bool IsAiredYet(const Item& item);
bool IsFinishedAiring(const Item& item);
int EstimateLastAiredEpisodeNumber(const Item& item);

bool IsItemOldEnough(const Item& item);
bool MetadataNeedsRefresh(const Item& item);

bool IsNsfw(const Item& item);

bool PlayEpisode(int anime_id, int number);
bool PlayLastEpisode(int anime_id);
bool PlayNextEpisode(int anime_id);
bool PlayNextEpisodeOfLastWatchedAnime();
bool PlayRandomAnime();
bool PlayRandomEpisode(int anime_id);
bool LinkEpisodeToAnime(Episode& episode, int anime_id);
void StartWatching(Item& item, Episode& episode);
void EndWatching(Item& item, Episode episode);
bool IsDeletedFromList(Item& item);
bool IsUpdateAllowed(Item& item, const Episode& episode, bool ignore_update_time);
void UpdateList(Item& item, Episode& episode);
void AddToQueue(Item& item, const Episode& episode, bool change_status);
void SetMyLastUpdateToNow(Item& item);

bool GetFansubFilter(int anime_id, std::vector<std::wstring>& groups);
bool SetFansubFilter(int anime_id, const std::wstring& group_name);

std::wstring GetImagePath(int anime_id = -1);

void GetUpcomingTitles(std::vector<int>& anime_ids);

bool IsInsideLibraryFolders(const std::wstring& path);
bool ValidateFolder(Item& item);

int GetEpisodeHigh(const Episode& episode);
int GetEpisodeLow(const Episode& episode);
std::wstring GetEpisodeRange(const Episode& episode);
std::wstring GetVolumeRange(const Episode& episode);
std::wstring GetEpisodeRange(const number_range_t& range);
bool IsAllEpisodesAvailable(const Item& item);
bool IsEpisodeRange(const Episode& episode);
bool IsValidEpisodeCount(int number);
bool IsValidEpisodeNumber(int number, int total);
bool IsValidEpisodeNumber(int number, int total, int watched);
std::wstring JoinEpisodeNumbers(const std::vector<int>& input);
void SplitEpisodeNumbers(const std::wstring& input, std::vector<int>& output);
int EstimateEpisodeCount(const Item& item);
void ChangeEpisode(int anime_id, int value);
void DecrementEpisode(int anime_id);
void IncrementEpisode(int anime_id);

void GetAllTitles(int anime_id, std::vector<std::wstring>& titles);
int GetMyRewatchedTimes(const Item& item);
void GetProgressRatios(const Item& item, float& ratio_aired, float& ratio_watched);

std::wstring TranslateMyStatus(int value, bool add_count);
std::wstring TranslateNumber(int value, const std::wstring& default_char = L"-");
std::wstring TranslateMyScore(int value, const std::wstring& default_char = L"-");
std::wstring TranslateMyScoreFull(int value);
std::wstring TranslateScore(double value);
std::wstring TranslateStatus(int value);
std::wstring TranslateType(int value);
int TranslateType(const std::wstring& value);

int TranslateResolution(const std::wstring& str, bool return_validity = false);

////////////////////////////////////////////////////////////////////////////////

bool IsValidDate(const Date& date);
std::wstring TranslateDate(const Date& date);
std::wstring TranslateDateToSeasonString(const Date& date);
std::wstring TranslateSeasonToMonths(const Season& season);

}  // namespace anime

#endif  // TAIGA_LIBRARY_ANIME_UTIL_H