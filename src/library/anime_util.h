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

#include "base/std.h"

class Date;

namespace anime {
  
class Episode;
class Item;

// Following functions are called when a new episode is recognized. Actual
// time depends on user settings.
void StartWatching(Item& item, Episode& episode);
void EndWatching(Item& item, Episode episode);
bool IsUpdateAllowed(Item& item, const Episode& episode, bool ignore_update_time);
void UpdateList(Item& item, Episode& episode);
void AddToQueue(Item& item, const Episode& episode, bool change_status);

bool GetFansubFilter(int anime_id, vector<wstring>& groups);
bool SetFansubFilter(int anime_id, const wstring& group_name);

wstring GetImagePath(int anime_id = -1);

void GetUpcomingTitles(vector<int>& anime_ids);

bool IsInsideRootFolders(const wstring& path);

bool IsValidEpisode(int episode, int watched, int total);
int EstimateEpisodeCount(const Item& item);

wstring TranslateMyStatus(int value, bool add_count);
wstring TranslateNumber(int value, const wstring& default_char = L"-");
wstring TranslateStatus(int value);
wstring TranslateType(int value);

int TranslateMyStatus(const wstring& value);
int TranslateStatus(const wstring& value);
int TranslateType(const wstring& value);

////////////////////////////////////////////////////////////////////////////////

bool IsValidDate(const Date& date);
void GetSeasonInterval(const wstring& season, Date& date_start, Date& date_end);
wstring TranslateDate(const Date& date);
wstring TranslateDateToSeason(const Date& date);
wstring TranslateSeasonToMonths(const wstring& season);

}  // namespace anime

#endif  // TAIGA_LIBRARY_ANIME_UTIL_H