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

#ifndef TAIGA_LIBRARY_ANIME_FILTER_H
#define TAIGA_LIBRARY_ANIME_FILTER_H

#include <map>
#include <string>
#include <vector>

namespace anime {

class Item;

class Filters {
public:
  Filters();
  virtual ~Filters() {}

  bool CheckItem(const Item& item, int text_index) const;
  void Reset();

  std::vector<bool> my_status;
  std::vector<bool> status;
  std::vector<bool> type;
  std::map<int, std::wstring> text;

private:
  bool FilterText(const Item& item, int text_index) const;
};

}  // namespace anime

#endif  // TAIGA_LIBRARY_ANIME_FILTER_H