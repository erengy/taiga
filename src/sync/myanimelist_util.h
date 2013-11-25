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

#ifndef TAIGA_SYNC_MYANIMELIST_UTIL_H
#define TAIGA_SYNC_MYANIMELIST_UTIL_H

#include "base/std.h"
#include "base/optional.h"

class Date;

namespace sync {
namespace myanimelist {

class AnimeValues {
public:
  Optional<int> episode;
  Optional<int> status;
  Optional<int> score;
  Optional<int> downloaded_episodes;
  Optional<int> storage_type;
  Optional<float> storage_value;
  Optional<int> times_rewatched;
  Optional<int> rewatch_value;
  Optional<wstring> date_start;
  Optional<wstring> date_finish;
  Optional<int> priority;
  Optional<int> enable_discussion;
  Optional<int> enable_rewatching;
  Optional<wstring> comments;
  Optional<wstring> fansub_group;
  Optional<wstring> tags;
};

wstring DecodeText(wstring text);
bool IsValidDate(const Date& date);
bool IsValidDate(const wstring& date);
bool IsValidEpisode(int episode, int watched, int total);
Date ParseDateString(const wstring& str);
void GetSeasonInterval(const wstring& season, Date& date_start, Date& date_end);

wstring TranslateDate(const Date& date);
wstring TranslateDateForApi(const Date& date);
Date TranslateDateFromApi(const wstring& date);
wstring TranslateDateToSeason(const Date& date);
wstring TranslateSeasonToMonths(const wstring& season);
wstring TranslateMyStatus(int value, bool add_count);
wstring TranslateNumber(int value, const wstring& default_char = L"-");
wstring TranslateStatus(int value);
wstring TranslateType(int value);

int TranslateMyStatus(const wstring& value);
int TranslateStatus(const wstring& value);
int TranslateType(const wstring& value);

void ViewAnimePage(int anime_id);
void ViewAnimeSearch(const wstring& title);
void ViewHistory();
void ViewMessages();
void ViewPanel();
void ViewProfile();
void ViewSeasonGroup();
void ViewUpcomingAnime();

}  // namespace myanimelist
}  // namespace sync

#endif  // TAIGA_SYNC_MYANIMELIST_UTIL_H