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

#include "base/json.h"
#include "sync/service.h"

class Date;

namespace anime {
enum class MyStatus;
enum class SeriesStatus;
enum class SeriesType;
class Item;
}

namespace sync::anilist {

enum class RatingSystem;

Date TranslateFuzzyDateFrom(const Json& json);
Json TranslateFuzzyDateTo(const Date& date);
std::string TranslateSeasonTo(const std::wstring& value);
anime::SeriesStatus TranslateSeriesStatusFrom(const std::string& value);
double TranslateSeriesRatingFrom(int value);
double TranslateSeriesRatingTo(double value);
anime::SeriesType TranslateSeriesTypeFrom(const std::string& value);
std::wstring TranslateMyRating(int value, RatingSystem rating_system);
anime::MyStatus TranslateMyStatusFrom(const std::string& value);
std::string TranslateMyStatusTo(anime::MyStatus value);
RatingSystem TranslateRatingSystemFrom(const std::string& value);

std::wstring GetAnimePage(const anime::Item& anime_item);
void RequestToken();
void ViewAnimePage(int anime_id);
void ViewProfile();
void ViewStats();

}  // namespace sync::anilist
