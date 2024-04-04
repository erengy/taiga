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

#include "sync/kitsu_types.h"
#include "sync/service.h"

namespace anime {
enum class AgeRating;
enum class MyStatus;
enum class SeriesStatus;
enum class SeriesType;
class Item;
}

namespace sync::kitsu {

RatingSystem GetRatingSystem();
std::vector<Rating> GetMyRatings(RatingSystem rating_system);

anime::AgeRating TranslateAgeRatingFrom(const std::string& value);
double TranslateSeriesRatingFrom(const std::string& value);
double TranslateSeriesRatingTo(double value);
anime::SeriesStatus TranslateSeriesStatusFrom(const std::string& value);
anime::SeriesType TranslateSeriesTypeFrom(const std::string& value);
std::wstring TranslateMyDateFrom(const std::string& value);
std::string TranslateMyDateTo(const std::wstring& value);
std::wstring TranslateMyLastUpdatedFrom(const std::string& value);
std::wstring TranslateMyRating(int value, RatingSystem rating_system);
int TranslateMyRatingFrom(int value);
int TranslateMyRatingTo(int value);
anime::MyStatus TranslateMyStatusFrom(const std::string& value);
std::string TranslateMyStatusTo(anime::MyStatus value);
RatingSystem TranslateRatingSystemFrom(const std::string& value);

std::wstring GetAnimePage(const anime::Item& anime_item);
void ViewAnimePage(int anime_id);
void ViewFeed();
void ViewLibrary();
void ViewProfile();

}  // namespace sync::kitsu
