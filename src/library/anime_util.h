/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

class Date;

namespace anime {
  
class Episode;
class Item;

bool IsAiredYet(const Item& item);
bool IsFinishedAiring(const Item& item);
int EstimateLastAiredEpisodeNumber(const Item& item);

bool CheckEpisodes(Item& item, int number = -1, bool check_folder = false);
bool CheckFolder(Item& item);
bool PlayEpisode(Item& item, int number);
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

bool IsValidEpisode(int episode, int watched, int total);
int EstimateEpisodeCount(const Item& item);

std::wstring TranslateMyStatus(int value, bool add_count);
std::wstring TranslateNumber(int value, const std::wstring& default_char = L"-");
std::wstring TranslateStatus(int value);
std::wstring TranslateType(int value);

int TranslateMyStatus(const std::wstring& value);
int TranslateStatus(const std::wstring& value);
int TranslateType(const std::wstring& value);

////////////////////////////////////////////////////////////////////////////////

bool IsValidDate(const Date& date);
void GetSeasonInterval(const std::wstring& season, Date& date_start, Date& date_end);
std::wstring TranslateDate(const Date& date);
std::wstring TranslateDateToSeason(const Date& date);
std::wstring TranslateSeasonToMonths(const std::wstring& season);

}  // namespace anime

#endif  // TAIGA_LIBRARY_ANIME_UTIL_H