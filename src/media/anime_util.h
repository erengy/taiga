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

#pragma once

#include <string>
#include <vector>

#include "media/anime.h"
#include "track/episode.h"

class Date;

namespace anime {

class Item;

bool IsValidId(int anime_id);

SeriesStatus GetAiringStatus(const Item& item);
bool IsAiredYet(const Item& item);
bool IsFinishedAiring(const Item& item);
int EstimateDuration(const Item& item);
int EstimateLastAiredEpisodeNumber(const Item& item);

bool IsItemOldEnough(const Item& item);
bool MetadataNeedsRefresh(const Item& item);

bool IsNsfw(const Item& item);

bool StartNewRewatch(int anime_id);
bool LinkEpisodeToAnime(Episode& episode, int anime_id);
void StartWatching(Item& item, Episode& episode);
void EndWatching(Item& item, Episode episode);
bool IsDeletedFromList(const Item& item);
bool IsUpdateAllowed(const Item& item, const Episode& episode, bool ignore_update_time);
void UpdateList(const Item& item, Episode& episode);
void AddToQueue(const Item& item, const Episode& episode, bool change_status);
void SetMyLastUpdateToNow(Item& item);

std::wstring GetImagePath(int anime_id = -1);

void GetUpcomingTitles(std::vector<int>& anime_ids);

bool IsInsideLibraryFolders(const std::wstring& path);
bool ValidateFolder(Item& item);

bool IsAllEpisodesAvailable(const Item& item);
bool IsValidEpisodeCount(int number);
bool IsValidEpisodeNumber(int number, int total);
bool IsValidEpisodeNumber(int number, int total, int watched);
int GetLastEpisodeNumber(const Item& item);
int EstimateEpisodeCount(const Item& item);

const std::wstring& GetPreferredTitle(const Item& item);
void GetAllTitles(int anime_id, std::vector<std::wstring>& titles);
std::vector<std::wstring> GetStudiosAndProducers(const Item& item);
void GetProgressRatios(const Item& item, float& ratio_aired, float& ratio_watched);

bool IsValidDate(const Date& date);

std::wstring NormalizeSynopsis(std::wstring str);

}  // namespace anime
