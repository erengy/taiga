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

#include "base/std.h"

#include "anime_filter.h"
#include "anime_item.h"

#include "base/string.h"

namespace anime {

// =============================================================================

Filters::Filters() {
  Reset();
}

bool Filters::CheckItem(Item& item) {
  // Filter my status
  for (size_t i = 0; i < my_status.size(); i++)
    if (!my_status.at(i) && item.GetMyStatus() == i)
      return false;

  // Filter airing status
  for (size_t i = 0; i < status.size(); i++)
    if (!status.at(i) && item.GetAiringStatus() == i + 1)
      return false;

  // Filter type
  for (size_t i = 0; i < type.size(); i++)
    if (!type.at(i) && item.GetType() == i + 1)
      return false;

  // Filter text
  vector<wstring> words;
  Split(text, L" ", words);
  RemoveEmptyStrings(words);
  for (auto it = words.begin(); it != words.end(); ++it) {
    if (InStr(item.GetTitle(), *it, 0, true) == -1 && 
        InStr(item.GetGenres(), *it, 0, true) == -1 && 
        InStr(item.GetMyTags(), *it, 0, true) == -1) {
      bool found = false;
      for (auto synonym = item.GetSynonyms().begin(); 
           !found && synonym != item.GetSynonyms().end(); ++synonym)
        if (InStr(*synonym, *it, 0, true) > -1) found = true;
      if (item.IsInList())
        for (auto synonym = item.GetUserSynonyms().begin(); 
             !found && synonym != item.GetUserSynonyms().end(); ++synonym)
          if (InStr(*synonym, *it, 0, true) > -1) found = true;
      if (!found) return false;
    }
  }

  // Item passed all filters
  return true;
}

void Filters::Reset() {
  my_status.clear();
  status.clear();
  type.clear();

  my_status.resize(7, true);
  status.resize(3, true);
  type.resize(6, true);

  text = L"";
}

} // namespace anime