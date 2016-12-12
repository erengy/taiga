/*
** Taiga
** Copyright (C) 2010-2016, Eren Okka
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

namespace anime {
class Item;
}

namespace sync {
namespace kitsu {
  
int TranslateAgeRatingFrom(const std::string& value);
double TranslateSeriesRatingFrom(double value);
double TranslateSeriesRatingTo(double value);
int TranslateSeriesTypeFrom(const std::string& value);
std::wstring TranslateDateFrom(const std::string& value);
std::wstring TranslateMyLastUpdatedFrom(const std::string& value);
int TranslateMyRatingFrom(const std::string& value);
std::string TranslateMyRatingTo(int value);
int TranslateMyStatusFrom(const std::string& value);
std::string TranslateMyStatusTo(int value);

std::wstring GetAnimePage(const anime::Item& anime_item);
void ViewAnimePage(int anime_id);
void ViewDashboard();
void ViewProfile();
void ViewRecommendations();
void ViewUpcomingAnime();

}  // namespace kitsu
}  // namespace sync
