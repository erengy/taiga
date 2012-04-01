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

#ifndef ANIMEDB_H
#define ANIMEDB_H

#include "std.h"

class Anime;

// =============================================================================

class AnimeSeasonDatabase {
public:
  AnimeSeasonDatabase();
  virtual ~AnimeSeasonDatabase() {}

  bool Load(wstring file);
  bool Save(wstring file = L"", bool minimal = false);

public:
  vector<Anime> items;
  time_t last_modified;
  bool modified;
  wstring name;

private:
  wstring file_, folder_;
};

extern AnimeSeasonDatabase SeasonDatabase;

#endif // ANIMEDB_H