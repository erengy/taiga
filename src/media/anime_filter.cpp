/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <map>
#include <regex>

#include "media/anime_filter.h"

#include "base/string.h"
#include "media/anime_item.h"
#include "media/anime_util.h"
#include "ui/translate.h"

namespace anime {

enum class SearchField {
  None,
  Id,
  Episodes,
  Title,
  Genre,
  Producer,
  Tag,
  Note,
  Type,
  Season,
  Year,
  Rewatch,
  Duration,
  Score,
  Debug,
};

enum class SearchOperator {
  EQ,
  GE,
  GT,
  LE,
  LT,
};

struct SearchTerm {
  SearchField field = SearchField::None;
  SearchOperator op = SearchOperator::EQ;
  std::wstring value;
};

SearchTerm GetSearchTerm(const std::wstring& str) {
  SearchTerm term;
  term.value = str;

  static const std::wregex pattern{LR"(([a-z]+):([!<=>]+)?(.+))"};

  static const std::map<std::wstring, SearchField> prefixes{
    {L"id", SearchField::Id},
    {L"eps", SearchField::Episodes},
    {L"title", SearchField::Title},
    {L"genre", SearchField::Genre},
    {L"producer", SearchField::Producer},
    {L"tag", SearchField::Tag},
    {L"note", SearchField::Note},
    {L"type", SearchField::Type},
    {L"season", SearchField::Season},
    {L"year", SearchField::Year},
    {L"rewatch", SearchField::Rewatch},
    {L"duration", SearchField::Duration},
    {L"score", SearchField::Score},
    {L"debug", SearchField::Debug},
  };

  static const std::map<std::wstring, SearchOperator> operators{
    {L"=", SearchOperator::EQ},
    {L">=", SearchOperator::GE},
    {L">", SearchOperator::GT},
    {L"<=", SearchOperator::LE},
    {L"<", SearchOperator::LT},
  };

  std::wsmatch matches;

  if (std::regex_match(str, matches, pattern)) {
    const auto it = prefixes.find(matches[1].str());
    if (it != prefixes.end()) {
      term.field = it->second;
      term.value = matches[3].str();
      if (matches[2].matched) {
        const auto it = operators.find(matches[2].str());
        if (it != operators.end()) {
          term.op = it->second;
        }
      }
    }
  }

  return term;
}

bool CheckNumber(const SearchOperator op, const int a, const int b) {
  switch (op) {
    default:
    case SearchOperator::EQ: return a == b;
    case SearchOperator::GE: return a >= b;
    case SearchOperator::GT: return a >  b;
    case SearchOperator::LE: return a <= b;
    case SearchOperator::LT: return a <  b;
  }
}

bool CheckString(const std::wstring& a, const std::wstring& b) {
  return InStr(a, b, 0, true) > -1;
}

bool CheckStrings(const std::vector<std::wstring>& v, const std::wstring& w) {
  for (const auto& s : v) {
    if (InStr(s, w, 0, true) > -1)
      return true;
  }
  return false;
};

////////////////////////////////////////////////////////////////////////////////

bool Filters::CheckItem(const Item& item, int text_index) const {
  const auto it = text.find(text_index);

  if (it == text.end() || it->second.empty())
    return true;

  std::vector<std::wstring> words;
  Split(it->second, L" ", words);
  RemoveEmptyStrings(words);

  std::vector<std::wstring> titles;
  GetAllTitles(item.GetId(), titles);

  const auto& genres = item.GetGenres();
  const auto& tags = item.GetTags();
  const auto& producers = item.GetProducers();
  const auto& studios = item.GetStudios();
  const auto& notes = item.GetMyNotes();

  std::vector<SearchTerm> search_terms;
  for (const auto& word : words) {
    search_terms.push_back(GetSearchTerm(word));
  }

  for (const auto& term : search_terms) {
    switch (term.field) {
      case SearchField::None:
        if (!CheckStrings(titles, term.value) &&
            !CheckStrings(genres, term.value) &&
            !CheckStrings(tags, term.value) &&
            !CheckString(notes, term.value)) {
          return false;
        }
        break;

      case SearchField::Id:
        if (!CheckNumber(term.op, item.GetId(), ToInt(term.value)))
          return false;
        break;

      case SearchField::Episodes:
        if (!CheckNumber(term.op, item.GetEpisodeCount(), ToInt(term.value)))
          return false;
        break;

      case SearchField::Title:
        if (!CheckStrings(titles, term.value))
          return false;
        break;

      case SearchField::Genre:
        if (!CheckStrings(genres, term.value))
          return false;
        break;

      case SearchField::Producer:
        if (!CheckStrings(producers, term.value) &&
            !CheckStrings(studios, term.value)) {
          return false;
        }
        break;

      case SearchField::Tag:
        if (!CheckStrings(tags, term.value)) {
          return false;
        }
        break;

      case SearchField::Note:
        if (!CheckString(notes, term.value))
          return false;
        break;

      case SearchField::Type:
        if (item.GetType() != ui::TranslateType(term.value))
          return false;
        break;

      case SearchField::Season: {
        const auto season =
            ui::TranslateDateToSeasonString(item.GetDateStart());
        if (!CheckString(season, term.value))
          return false;
        break;
      }

      case SearchField::Year: {
        const auto year = item.GetDateStart().year();
        if (!CheckNumber(term.op, year, ToInt(term.value)))
          return false;
        break;
      }

      case SearchField::Rewatch: {
        const auto rewatches = item.GetMyRewatchedTimes();
        if (!CheckNumber(term.op, rewatches, ToInt(term.value)))
          return false;
        break;
      }

      case SearchField::Duration: {
        const auto duration = item.GetEpisodeLength();
        if (!CheckNumber(term.op, duration, ToInt(term.value)))
          return false;
        break;
      }

      case SearchField::Score: {
        const auto score = item.GetMyScore();
        if (!CheckNumber(term.op, score, ToInt(term.value)))
          return false;
        break;
      }

      case SearchField::Debug: {
        if (term.value == L"watched") {
          const int eps_watched = item.GetMyLastWatchedEpisode();
          const int eps_total = item.GetEpisodeCount();
          if (!anime::IsValidEpisodeNumber(eps_watched, eps_total) ||
              (eps_watched < eps_total &&
               item.GetMyStatus() == anime::MyStatus::Completed)) {
            continue;
          } else {
            return false;
          }
        } else {
          return false;
        }
        break;
      }
    }
  }

  return true;
}

}  // namespace anime
