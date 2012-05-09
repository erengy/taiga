/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#ifndef MYANIMELIST_H
#define MYANIMELIST_H

#include "std.h"
#include "optional.h"

namespace anime {
class ListItem;
}
class Date;
class EventItem;
class HttpClient;

namespace mal {

// =============================================================================

const COLORREF COLOR_DARKBLUE = RGB(46, 81, 162);
const COLORREF COLOR_LIGHTBLUE = RGB(225, 231, 245);
const COLORREF COLOR_LIGHTGRAY = RGB(246, 246, 246);

enum MyStatus {
  MYSTATUS_NOTINLIST = 0,
  MYSTATUS_WATCHING,
  MYSTATUS_COMPLETED,
  MYSTATUS_ONHOLD,
  MYSTATUS_DROPPED,
  MYSTATUS_UNKNOWN,
  MYSTATUS_PLANTOWATCH
};

enum RewatchValue {
  REWATCH_VERYLOW = 1,
  REWATCH_LOW,
  REWATCH_MEDIUM,
  REWATCH_HIGH,
  REWATCH_VERYHIGH
};

enum Status {
  STATUS_AIRING = 1,
  STATUS_FINISHED,
  STATUS_NOTYETAIRED
};

enum StorageType {
  STORAGE_HARDDRIVE = 1,
  STORAGE_DVDCD,
  STORAGE_NONE,
  STORAGE_RETAILDVD,
  STORAGE_VHS,
  STORAGE_EXTERNALHD,
  STORAGE_NAS
};

enum Type {
  TYPE_TV = 1,
  TYPE_OVA,
  TYPE_MOVIE,
  TYPE_SPECIAL,
  TYPE_ONA,
  TYPE_MUSIC
};

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

// =============================================================================

bool AskToDiscuss(int anime_id, int episode_number);
void CheckProfile();
bool DownloadImage(int anime_id, const wstring& image_url, class HttpClient* client = nullptr);
bool DownloadUserImage(bool thumb);
bool GetAnimeDetails(int anime_id, class HttpClient* client = nullptr);
bool GetList(bool login);
bool Login();
bool ParseAnimeDetails(const wstring& data);
bool ParseSearchResult(const wstring& data, int anime_id = 0);
bool SearchAnime(int anime_id, wstring title, class HttpClient* client = nullptr);
bool Update(AnimeValues& anime_values, int anime_id, int update_mode);
bool UpdateSucceeded(EventItem& item, const wstring& data, int status_code);

wstring DecodeText(wstring text);
bool IsValidDate(const Date& date);
bool IsValidDate(const wstring& date);
bool IsValidEpisode(int episode, int watched, int total);
Date ParseDateString(const wstring& str);

wstring TranslateDate(const Date& date);
wstring TranslateDateForApi(const Date& date);
Date TranslateDateFromApi(const wstring& date);
wstring TranslateDateToSeason(const Date& date);
wstring TranslateMyStatus(int value, bool add_count);
wstring TranslateNumber(int value, const wstring& default_char = L"-");
wstring TranslateRewatchValue(int value);
wstring TranslateStatus(int value);
wstring TranslateStorageType(int value);
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

} // namespace mal

#endif // MYANIMELIST_H