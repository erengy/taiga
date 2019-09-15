/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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

#pragma once

#include <string>

#include "media/anime_season.h"

namespace library {

class SeasonDatabase {
public:
  bool Load(const anime::Season& season);

  // Checkes if a significant portion of season data is empty and requires
  // refreshing.
  bool IsRefreshRequired();

  void Reset();

  // Improves season data by excluding invalid items (i.e. postpones series) and
  // adding missing ones from the anime database.
  void Review(bool hide_nsfw = true);

  // Only IDs are stored here, actual info is kept in anime::Database.
  std::vector<int> items;

  // Current season (e.g. "Spring 2012")
  anime::Season current_season;
};

}  // namespace library

extern library::SeasonDatabase SeasonDatabase;
