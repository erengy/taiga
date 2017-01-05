/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#include "base/string.h"
#include "library/anime_filter.h"
#include "library/anime_item.h"
#include "library/anime_util.h"

namespace anime {

Filters::Filters() {
  Reset();
}

bool Filters::CheckItem(const Item& item, int text_index) const {
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
  if (!FilterText(item, text_index))
    return false;

  // Item passed all filters
  return true;
}

bool Filters::FilterText(const Item& item, int text_index) const {
  std::vector<std::wstring> words;
  auto it = text.find(text_index);
  auto filter_text = it != text.end() ? it->second : std::wstring();
  Split(filter_text, L" ", words);
  RemoveEmptyStrings(words);

  std::vector<std::wstring> titles;
  GetAllTitles(item.GetId(), titles);

  const auto& genres = item.GetGenres();

  for (const auto& word : words) {
    auto check_strings = [&word](const std::vector<std::wstring>& v) {
      for (const auto& str : v) {
        if (InStr(str, word, 0, true) > -1)
          return true;
      }
      return false;
    };
    if (!check_strings(titles) &&
        !check_strings(genres) &&
        InStr(item.GetMyTags(), word, 0, true) == -1)
      return false;
  }

  return true;
}

void Filters::Reset() {
  my_status.clear();
  status.clear();
  type.clear();

  my_status.resize(7, true);
  status.resize(3, true);
  type.resize(6, true);

  text.clear();
}

}  // namespace anime