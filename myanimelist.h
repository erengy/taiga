/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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

enum MAL_MyStatus {
  MAL_NOTINLIST,
  MAL_WATCHING,
  MAL_COMPLETED,
  MAL_ONHOLD,
  MAL_DROPPED,
  MAL_UNKNOWN,
  MAL_PLANTOWATCH
};

enum MAL_Status {
  MAL_AIRING = 1,
  MAL_FINISHED,
  MAL_NOTYETAIRED
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

class CAnime;

class CMyAnimeList {
public:
  CMyAnimeList();
  ~CMyAnimeList() {};

  void CheckProfile();
  void DownloadImage(CAnime* pAnimeItem);
  bool GetList(bool login);
  bool Login();
  BOOL SearchAnime(wstring title, CAnime* pAnimeItem = NULL);
  void Update(int index, int id, int episode, int score, int status, wstring tags, int mode);
  
  bool IsValidEpisode(int episode, int watched, int total);
  wstring TranslateDate(wstring value, bool reverse = false);
  wstring TranslateMyStatus(int value, bool add_count);
  int TranslateMyStatus(const wstring& value);
  wstring TranslateNumber(int value, LPCWSTR default_char = L"-");
  wstring TranslateStatus(int value);
  int TranslateStatus(const wstring& value);
  wstring TranslateType(int value);
  int TranslateType(const wstring& value);
  void ViewAnimePage(int index);
  void ViewAnimeSearch(wstring title);
  void ViewHistory();
  void ViewMessages();
  void ViewPanel();
  void ViewProfile();
};

extern CMyAnimeList MAL;

#endif // MYANIMELIST_H