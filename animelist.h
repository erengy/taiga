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

#ifndef ANIMELIST_H
#define ANIMELIST_H

#include "std.h"
#include "anime.h"

enum AnimeListWriteMode {
  ANIMELIST_EDITUSER,
  ANIMELIST_EDITANIME,
  ANIMELIST_ADDANIME,
  ANIMELIST_DELETEANIME
};

// =============================================================================

class MalUser {
public:
  int id, watching, completed, on_hold, dropped, plan_to_watch;
  wstring name, days_spent_watching;
};

class User : public MalUser {
public:
  User();
  virtual ~User() {}

  void Clear();
  int GetItemCount(int status);
  void IncreaseItemCount(int status, int count = 1, bool write = true);
};

// =============================================================================

class AnimeList {
public:
  AnimeList();
  ~AnimeList() {};

  void AddItem(Anime& anime);
  void AddItem(int series_id, 
    const wstring& series_title, 
    const wstring& series_synonyms, 
    int series_type, 
    int series_episodes, 
    int series_status, 
    const wstring& series_start, 
    const wstring& series_end, 
    const wstring& series_image, 
    int my_id, 
    int my_watched_episodes, 
    const wstring& my_start_date, 
    const wstring& my_finish_date, 
    int my_score, 
    int my_status, 
    int my_rewatching, 
    int my_rewatching_ep, 
    const wstring& my_last_updated, 
    const wstring& my_tags, 
    bool resize = true);
  
  void Clear();
  bool Read();
  bool Save(int anime_id, wstring child, wstring value, 
    int mode = ANIMELIST_EDITANIME);
  
  bool DeleteItem(int anime_id);
  Anime* FindItem(int anime_id);
  int FindItemIndex(int anime_id);

public:
  User user;
  vector<Anime> items;
  int index, count;

  class ListFilters {
  public:
    ListFilters();
    virtual ~ListFilters() {}
    bool Check(Anime& anime);
    void Reset();
  public:
    bool my_status[6], status[3], type[6], new_episodes;
    wstring text;
  } filters;
};

extern AnimeList AnimeList;

#endif // ANIMELIST_H