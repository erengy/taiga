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

#include "sync/service.h"

class Date;

namespace anime {
enum class AgeRating;
enum class MyStatus;
enum class SeriesStatus;
enum class SeriesType;
class Item;
}

namespace sync::myanimelist {

std::vector<Rating> GetMyRatings();

anime::AgeRating TranslateAgeRatingFrom(const std::wstring& value);
Date TranslateDateFrom(const std::wstring& value);
int TranslateEpisodeLengthFrom(int value);
anime::SeriesStatus TranslateSeriesStatusFrom(const std::wstring& value);
anime::SeriesType TranslateSeriesTypeFrom(const std::wstring& value);
std::wstring TranslateMyLastUpdatedFrom(const std::string& value);
std::wstring TranslateMyRating(int value, bool full);
int TranslateMyRatingFrom(int value);
int TranslateMyRatingTo(int value);
anime::MyStatus TranslateMyStatusFrom(const std::wstring& value);
std::string TranslateMyStatusTo(anime::MyStatus value);

std::wstring GetAnimePage(const anime::Item& anime_item);
void RequestAuthorizationCode(std::wstring& code_verifier);
void ViewAnimePage(int anime_id);
void ViewAnimeSearch(const std::wstring& title);
void ViewHistory();
void ViewPanel();
void ViewProfile();
void ViewUpcomingAnime();

}  // namespace sync::myanimelist
