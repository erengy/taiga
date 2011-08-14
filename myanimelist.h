/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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

#define MAL_DARKBLUE RGB(46, 81, 162)
#define MAL_LIGHTBLUE RGB(225, 231, 245)
#define MAL_LIGHTGRAY RGB(246, 246, 246)

enum MAL_MyStatus {
  MAL_NOTINLIST,
  MAL_WATCHING,
  MAL_COMPLETED,
  MAL_ONHOLD,
  MAL_DROPPED,
  MAL_UNKNOWN,
  MAL_PLANTOWATCH
};

enum MAL_RewatchValue {
  MAL_REWATCH_VERYLOW = 1,
  MAL_REWATCH_LOW,
  MAL_REWATCH_MEDIUM,
  MAL_REWATCH_HIGH,
  MAL_REWATCH_VERYHIGH
};

enum MAL_Status {
  MAL_AIRING = 1,
  MAL_FINISHED,
  MAL_NOTYETAIRED
};

enum MAL_StorageType {
  MAL_STORAGE_HARDDRIVE = 1,
  MAL_STORAGE_DVDCD,
  MAL_STORAGE_NONE,
  MAL_STORAGE_RETAILDVD,
  MAL_STORAGE_VHS,
  MAL_STORAGE_EXTERNALHD,
  MAL_STORAGE_NAS
};

enum MAL_Type {
  MAL_TV = 1,
  MAL_OVA,
  MAL_MOVIE,
  MAL_SPECIAL,
  MAL_ONA,
  MAL_MUSIC
};

// =============================================================================

class Anime;
class EventItem;
class HttpClient;

class MalAnimeValues {
public:
  MalAnimeValues();
  virtual ~MalAnimeValues() {};
  
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

class MyAnimeList {
public:
  MyAnimeList() {};
  virtual ~MyAnimeList() {};

  void CheckProfile();
  bool DownloadImage(Anime* anime, class HttpClient* client = nullptr);
  bool GetAnimeDetails(Anime* anime, class HttpClient* client = nullptr);
  bool GetList(bool login);
  bool Login();
  bool ParseAnimeDetails(const wstring& data, Anime* anime = nullptr);
  bool ParseSearchResult(const wstring& data, Anime* anime = nullptr);
  bool SearchAnime(wstring title, Anime* anime = NULL, class HttpClient* client = nullptr);
  bool Update(MalAnimeValues& anime_values, int anime_id, int update_mode);
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
};

extern MyAnimeList MAL;

#endif // MYANIMELIST_H