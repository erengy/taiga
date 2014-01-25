/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#ifndef TAIGA_LIBRARY_DISCOVER_H
#define TAIGA_LIBRARY_DISCOVER_H

#include <string>

namespace library {

class SeasonDatabase {
public:
  // Loads season data from db\season\<seasonname>.xml, returns false if no such
  // file exists.
  bool Load(std::wstring file);

  // Checkes if a significant portion of season data is empty and requires 
  // refreshing.
  bool IsRefreshRequired();

  // Improves season data by excluding invalid items (i.e. postpones series) and 
  // adding missing ones from the anime database.
  void Review(bool hide_hentai = true);

  // Only IDs are stored here, actual info is kept in anime::Database.
  std::vector<int> items;

  // Season name (e.g. "Spring 2012")
  std::wstring name;
};

}  // namespace library

extern library::SeasonDatabase SeasonDatabase;

#endif // TAIGA_LIBRARY_DISCOVER_H