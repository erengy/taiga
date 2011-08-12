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

#ifndef ANIMEDB_H
#define ANIMEDB_H

#include "std.h"
#include "anime.h"

// =============================================================================

class CAnimeDatabase {
public:
  CAnimeDatabase() {}
  virtual ~CAnimeDatabase() {}
};

extern CAnimeDatabase AnimeDatabase;

// =============================================================================

class CAnimeSeasonDatabase {
public:
  CAnimeSeasonDatabase() : Modified(false) {}
  virtual ~CAnimeSeasonDatabase() {}

  bool Read(wstring file);
  bool Write(wstring file = L"");
  bool WriteForRelease();

public:
  vector<CAnime> Items;
  wstring LastModified, Name;
  wstring File, Folder;
  bool Modified;
};

extern CAnimeSeasonDatabase SeasonDatabase;

#endif // ANIMEDB_H