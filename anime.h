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

#ifndef ANIME_H
#define ANIME_H

#include "std.h"

#include "time.h"

namespace anime {

// =============================================================================

// ID_NOTINLIST
//   Used in Episode data to denote the item is not in user's list.
//   Item may or may not be in the database.
//
// ID_UNKNOWN
//   There's no item in the database with this ID.
//   This is the default ID for all anime items.
//
enum AnimeId {
  ID_NOTINLIST = -1,
  ID_UNKNOWN = 0
};

// GetDate() and SetDate() require a particular type.
//
// DATE_START
//   Airing date of the first episode (i.e., season premiere for TV series).
//
// DATE_END
//   Airing date of the last episode (i.e., season finale for TV series).
//   The same as DATE_START for one-episode titles.
//
enum DateType {
  DATE_START = 0,
  DATE_END
};

// All anime items have series information.
class SeriesInformation {
 public:
  SeriesInformation();
  virtual ~SeriesInformation() {}

  int id;
  int type;
  int episodes;
  int status;
  wstring title;
  vector<wstring> synonyms;
  Date date_start;
  Date date_end;
  wstring image_url;

  wstring english_title;
  wstring genres;
  wstring popularity;
  wstring producers;
  wstring rank;
  wstring score;
  wstring synopsis;

  wstring clean_english_title;
  wstring clean_title;
  vector<wstring> clean_synonyms;
};

// Invalid for anime items that are not in user's list.
class MyInformation {
 public:
  MyInformation();
  virtual ~MyInformation() {}

  int watched_episodes;
  int score;
  int status;
  int rewatching;
  int rewatching_ep;
  Date date_start;
  Date date_finish;
  wstring last_updated;
  wstring tags;
  
  vector<bool> available_episodes;
  wstring new_episode_path;
  wstring folder;
  vector<wstring> synonyms;
  vector<wstring> clean_synonyms;
  bool playing;
};

bool GetFansubFilter(int anime_id, vector<wstring>& groups);
bool SetFansubFilter(int anime_id, const wstring& group_name);

wstring GetImagePath(int anime_id = -1);

} // namespace anime

#endif // ANIME_H