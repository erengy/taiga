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

#ifndef TAIGA_SYNC_HUMMINGBIRD_UTIL_H
#define TAIGA_SYNC_HUMMINGBIRD_UTIL_H

#include <string>

namespace anime {
class Item;
}

namespace sync {
namespace hummingbird {
  
int TranslateAgeRatingFrom(const std::wstring& value);
double TranslateSeriesRatingFrom(float value);
double TranslateSeriesRatingTo(double value);
int TranslateSeriesStatusFrom(int value);
int TranslateSeriesStatusFrom(const std::wstring& value);
int TranslateSeriesTypeFrom(int value);
int TranslateSeriesTypeFrom(const std::wstring& value);
std::wstring TranslateDateFrom(const std::wstring& value);
int TranslateMyRatingFrom(const std::wstring& value, const std::wstring& type);
std::wstring TranslateMyRatingTo(int value);
int TranslateMyStatusFrom(const std::wstring& value);
std::wstring TranslateMyStatusTo(int value);

std::wstring GetAnimePage(const anime::Item& anime_item);
void ViewAnimePage(int anime_id);
void ViewDashboard();
void ViewProfile();
void ViewRecommendations();
void ViewUpcomingAnime();

}  // namespace hummingbird
}  // namespace sync

#endif  // TAIGA_SYNC_HUMMINGBIRD_UTIL_H