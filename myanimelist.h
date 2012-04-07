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

class Anime;
class EventItem;
class HttpClient;

namespace mal {

const COLORREF COLOR_DARKBLUE = RGB(46, 81, 162);
const COLORREF COLOR_LIGHTBLUE RGB(225, 231, 245);
const COLORREF COLOR_LIGHTGRAY RGB(246, 246, 246);

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

// =============================================================================

class AnimeValues {
public:
  AnimeValues();
  virtual ~AnimeValues() {};
  
  int episode;
  int status;
  int score;
  int downloaded_episodes;
  int storage_type;
  float storage_value;
  int times_rewatched;
  int rewatch_value;
  wstring date_start;
  wstring date_finish;
  int priority;
  int enable_discussion;
  int enable_rewatching;
  wstring comments;
  wstring fansub_group;
  wstring tags;
};

// =============================================================================

bool AskToDiscuss(Anime* anime, int episode_number);
void CheckProfile();
bool DownloadImage(Anime* anime, class HttpClient* client = nullptr);
bool GetAnimeDetails(Anime* anime, class HttpClient* client = nullptr);
bool GetList(bool login);
bool Login();
bool ParseAnimeDetails(const wstring& data, Anime* anime = nullptr);
bool ParseSearchResult(const wstring& data, Anime* anime = nullptr);
bool SearchAnime(wstring title, Anime* anime = NULL, class HttpClient* client = nullptr);
bool Update(AnimeValues& anime_values, int anime_id, int update_mode);
bool UpdateSucceeded(EventItem& item, const wstring& data, int status_code);

void DecodeText(wstring& text);
bool IsValidDate(const wstring& date);
bool IsValidEpisode(int episode, int watched, int total);
void ParseDateString(const wstring& date, unsigned short& year, unsigned short& month, unsigned short& day);

wstring TranslateDate(wstring value);
wstring TranslateDateToSeason(wstring value);
wstring TranslateMyStatus(int value, bool add_count);
wstring TranslateNumber(int value, LPCWSTR default_char = L"-");
wstring TranslateRewatchValue(int value);
wstring TranslateStatus(int value);
wstring TranslateStorageType(int value);
wstring TranslateType(int value);

int TranslateMyStatus(const wstring& value);
int TranslateStatus(const wstring& value);
int TranslateType(const wstring& value);

void ViewAnimePage(int series_id);
void ViewAnimeSearch(wstring title);
void ViewHistory();
void ViewMessages();
void ViewPanel();
void ViewProfile();
void ViewSeasonGroup();

} // namespace mal

#endif // MYANIMELIST_H