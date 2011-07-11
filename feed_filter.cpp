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

#include "std.h"
#include "common.h"
#include "feed.h"
#include "myanimelist.h"
#include "string.h"

// =============================================================================

bool EvaluateAction(int action, bool condition) {
  switch (action) {
    case FEED_FILTER_ACTION_EXCLUDE:
      return !condition;
    case FEED_FILTER_ACTION_INCLUDE:
      return condition;
    case FEED_FILTER_ACTION_PREFER:
      return condition;
    default:
      return true;
  }
}

bool EvaluateCondition(const CFeedFilterCondition& condition, const CFeedItem& item) {
  bool is_numeric = false;
  int anime_index = item.EpisodeData.Index;
  wstring element, value = ReplaceVariables(condition.Value, item.EpisodeData);

  switch (condition.Element) {
    case FEED_FILTER_ELEMENT_TITLE:
      element = item.Title;
      break;
    case FEED_FILTER_ELEMENT_CATEGORY:
      element = item.Category;
      break;
    case FEED_FILTER_ELEMENT_LINK:
      element = item.Link;
      break;
    case FEED_FILTER_ELEMENT_DESCRIPTION:
      element = item.Description;
      break;
    case FEED_FILTER_ELEMENT_ANIME_TITLE:
      element = item.EpisodeData.Title;
      break;
    case FEED_FILTER_ELEMENT_ANIME_GROUP:
      element = item.EpisodeData.Group;
      break;
    case FEED_FILTER_ELEMENT_ANIME_EPISODE:
      element = ToWSTR(GetEpisodeHigh(item.EpisodeData.Number));
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_ANIME_EPISODEAVAILABLE:
      if (anime_index > 0 && anime_index <= AnimeList.Count) {
        int number = GetEpisodeHigh(item.EpisodeData.Number);
        element = ToWSTR(AnimeList.Item[anime_index].IsEpisodeAvailable(number));
        is_numeric = true;
      }
      break;
    case FEED_FILTER_ELEMENT_ANIME_RESOLUTION:
      element = item.EpisodeData.Resolution;
      break;
    case FEED_FILTER_ELEMENT_ANIME_ID:
      if (anime_index > 0 && anime_index <= AnimeList.Count) {
        element = ToWSTR(AnimeList.Item[anime_index].Series_ID);
        is_numeric = true;
      }
      break;
    case FEED_FILTER_ELEMENT_ANIME_MYSTATUS:
      if (anime_index > 0 && anime_index <= AnimeList.Count) {
        element = ToWSTR(AnimeList.Item[anime_index].GetStatus());
        is_numeric = true;
      }
      break;
    case FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS:
      if (anime_index > 0 && anime_index <= AnimeList.Count) {
        element = ToWSTR(AnimeList.Item[anime_index].GetAiringStatus());
        is_numeric = true;
      }
      break;
  }

  switch (condition.Operator) {
    case FEED_FILTER_OPERATOR_IS:
      if (is_numeric) {
        if (IsEqual(value, L"True")) return ToINT(element) == TRUE;
        return ToINT(element) == ToINT(value);
      } else {
        return IsEqual(element, value);
      }
    case FEED_FILTER_OPERATOR_ISNOT:
      if (is_numeric) {
        if (IsEqual(value, L"True")) return ToINT(element) == TRUE;
        return ToINT(element) != ToINT(value);
      } else {
        return !IsEqual(element, value);
      }
    case FEED_FILTER_OPERATOR_ISGREATERTHAN:
      if (is_numeric) {
        return ToINT(element) > ToINT(value);
      } else {
        return CompareStrings(element, condition.Value) > 0;
      }
    case FEED_FILTER_OPERATOR_ISLESSTHAN:
      if (is_numeric) {
        return ToINT(element) < ToINT(value);
      } else {
        return CompareStrings(element, condition.Value) < 0;
      }
    case FEED_FILTER_OPERATOR_BEGINSWITH:
      return StartsWith(element, value);
    case FEED_FILTER_OPERATOR_ENDSWITH:
      return EndsWith(element, value);
    case FEED_FILTER_OPERATOR_CONTAINS:
      return InStr(element, value, 0, true) > -1;
    case FEED_FILTER_OPERATOR_CONTAINSNOT:
      return InStr(element, value, 0, true) == -1;
  }

  return false;
}

// =============================================================================

CFeedFilter& CFeedFilter::operator=(const CFeedFilter& filter) {
  Action = filter.Action;
  Enabled = filter.Enabled;
  Match = filter.Match;
  Name = filter.Name;

  Conditions.resize(filter.Conditions.size());
  std::copy(filter.Conditions.begin(), filter.Conditions.end(), Conditions.begin());

  return *this;
}

void CFeedFilter::AddCondition(int element, int op, const wstring& value) {
  Conditions.resize(Conditions.size() + 1);
  Conditions.back().Element = element;
  Conditions.back().Operator = op;
  Conditions.back().Value = value;
}

bool CFeedFilter::Filter(CFeed& feed, CFeedItem& item, bool recursive) {
  if (!Enabled) return true;
  
  bool condition = false;
  size_t index = 0;
  
  switch (Match) {
    case FEED_FILTER_MATCH_ALL:
      condition = true;
      for (size_t i = 0; i < Conditions.size(); i++) {
        if (!EvaluateCondition(Conditions[i], item)) {
          condition = false;
          index = i;
          break;
        }
      }
      break;
    case FEED_FILTER_MATCH_ANY:
      condition = false;
      for (size_t i = 0; i < Conditions.size(); i++) {
        if (EvaluateCondition(Conditions[i], item)) {
          condition = true;
          index = i;
          break;
        }
      }
      break;
  }
  
  if (EvaluateAction(Action, condition)) {
    // Handle preferences
    if (Action == FEED_FILTER_ACTION_PREFER && recursive) {
      for (auto it = feed.Item.begin(); it != feed.Item.end(); ++it) {
        // Do not bother if the item failed before
        if (it->Download == false) continue;
        // Do not filter the same item again
        if (it->Index == item.Index) continue;
        // Is it the same title?
        if (it->EpisodeData.Index > -1 && it->EpisodeData.Index != item.EpisodeData.Index) continue;
        if (it->EpisodeData.Index == -1 && !IsEqual(it->EpisodeData.Title, item.EpisodeData.Title)) continue;
        // Is it the same episode?
        if (it->EpisodeData.Number != item.EpisodeData.Number) continue;
        // Try applying the same filter
        if (!this->Filter(feed, *it, false)) {
          // Hey, we don't prefer your kind around here!
          it->Download = false;
        }
      }
    }

    // Item passed the filter (tick!)
    return true;
  
  } else {
    if (Action == FEED_FILTER_ACTION_PREFER && recursive) return true;

    #ifdef _DEBUG
    item.Description = L"!FILTER :: " + 
      Aggregator.FilterManager.TranslateConditions(*this, index) +
      L" -- " + item.Description;
    #endif
    
    // Out you go, you pathetic feed item!
    return false;
  }
}

// =============================================================================

void CFeedFilterManager::AddDefaultFilters() {
  // Discard unknown titles
  AddFilter(FEED_FILTER_ACTION_EXCLUDE, FEED_FILTER_MATCH_ALL, true, L"Discard unknown titles");
  Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_ID, FEED_FILTER_OPERATOR_IS, L"");
  // Discard completed titles
  AddFilter(FEED_FILTER_ACTION_EXCLUDE, FEED_FILTER_MATCH_ALL, true, L"Discard completed titles");
  Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_MYSTATUS, FEED_FILTER_OPERATOR_IS, ToWSTR(MAL_COMPLETED));
  // Discard dropped titles
  AddFilter(FEED_FILTER_ACTION_EXCLUDE, FEED_FILTER_MATCH_ALL, true, L"Discard dropped titles");
  Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_MYSTATUS, FEED_FILTER_OPERATOR_IS, ToWSTR(MAL_DROPPED));
  
  // Discard watched episodes
  AddFilter(FEED_FILTER_ACTION_EXCLUDE, FEED_FILTER_MATCH_ANY, true, L"Discard watched episodes");
  Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_EPISODE, FEED_FILTER_OPERATOR_IS, L"%watched%");
  Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_EPISODE, FEED_FILTER_OPERATOR_ISLESSTHAN, L"%watched%");
  Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_EPISODEAVAILABLE, FEED_FILTER_OPERATOR_IS, L"True");
  
  // Prefer high resolution files
  AddFilter(FEED_FILTER_ACTION_PREFER, FEED_FILTER_MATCH_ANY, true, L"Prefer high resolution files");
  Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_RESOLUTION, FEED_FILTER_OPERATOR_CONTAINS, L"720p");
  Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_RESOLUTION, FEED_FILTER_OPERATOR_CONTAINS, L"1280x720");
}

void CFeedFilterManager::AddFilter(int action, int match, bool enabled, const wstring& name) {
  Filters.resize(Filters.size() + 1);
  Filters.back().Action = action;
  Filters.back().Enabled = enabled;
  Filters.back().Match = match;
  Filters.back().Name = name;
}

int CFeedFilterManager::Filter(CFeed& feed) {
  bool download = true;
  int anime_index = -1, count = 0, number = 0;
  
  for (auto item = feed.Item.begin(); item != feed.Item.end(); ++item) {
    download = true;
    anime_index = item->EpisodeData.Index;
    number = GetEpisodeHigh(item->EpisodeData.Number);
    
    if (anime_index > 0 && number > AnimeList.Item[anime_index].GetLastWatchedEpisode()) {
      item->EpisodeData.NewEpisode = true;
    }

    if (!item->Download) continue;
      
    // Apply filters
    for (size_t j = 0; j < Filters.size(); j++) {
      if (!Filters[j].Filter(feed, *item, true)) {
        download = false;
        break;
      }
    }
      
    // Check archive
    if (download) {
      download = !Aggregator.SearchArchive(item->Title);
    }

    // Mark item
    if (download) {
      item->Download = true;
      count++;
    } else {
      item->Download = false;
    }
  }

  return count;
}

// =============================================================================

wstring CFeedFilterManager::TranslateCondition(const CFeedFilterCondition& condition) {
  wstring value;

  switch (condition.Element) {
    case FEED_FILTER_ELEMENT_ANIME_MYSTATUS:
      value = MAL.TranslateMyStatus(ToINT(condition.Value), false);
      break;
    case FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS:
      value = MAL.TranslateStatus(ToINT(condition.Value));
      break;
    default:
      value = condition.Value;
  }

  return TranslateElement(condition.Element) + L" " + 
    TranslateOperator(condition.Operator) + L" \"" + 
    value + L"\"";
}

wstring CFeedFilterManager::TranslateConditions(const CFeedFilter& filter, size_t index) {
  wstring str;
  
  size_t max_index = filter.Match == FEED_FILTER_MATCH_ALL ? 
    filter.Conditions.size() : index + 1;
  
  for (size_t i = index; i < max_index; i++) {
    if (i > index) str += L" & ";
    str += TranslateCondition(filter.Conditions[i]);
  }

  return str;
}

wstring CFeedFilterManager::TranslateElement(int element) {
  switch (element) {
    case FEED_FILTER_ELEMENT_TITLE:
      return L"Title";
    case FEED_FILTER_ELEMENT_CATEGORY:
      return L"Category";
    case FEED_FILTER_ELEMENT_LINK:
      return L"Link";
    case FEED_FILTER_ELEMENT_DESCRIPTION:
      return L"Description";
    case FEED_FILTER_ELEMENT_ANIME_TITLE:
      return L"Anime title";
    case FEED_FILTER_ELEMENT_ANIME_GROUP:
      return L"Anime group";
    case FEED_FILTER_ELEMENT_ANIME_EPISODE:
      return L"Anime episode";
    case FEED_FILTER_ELEMENT_ANIME_EPISODEAVAILABLE:
      return L"Anime episode availability";
    case FEED_FILTER_ELEMENT_ANIME_RESOLUTION:
      return L"Anime resolution";
    case FEED_FILTER_ELEMENT_ANIME_ID:
      return L"Anime ID";
    case FEED_FILTER_ELEMENT_ANIME_MYSTATUS:
      return L"Anime watching status";
    case FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS:
      return L"Anime airing status";
    default:
      return L"?";
  }
}

wstring CFeedFilterManager::TranslateOperator(int op) {
  switch (op) {
    case FEED_FILTER_OPERATOR_IS:
      return L"is";
    case FEED_FILTER_OPERATOR_ISNOT:
      return L"is not";
    case FEED_FILTER_OPERATOR_ISGREATERTHAN:
      return L"is greater than";
    case FEED_FILTER_OPERATOR_ISLESSTHAN:
      return L"is less than";
    case FEED_FILTER_OPERATOR_BEGINSWITH:
      return L"begins with";
    case FEED_FILTER_OPERATOR_ENDSWITH:
      return L"ends with";
    case FEED_FILTER_OPERATOR_CONTAINS:
      return L"contains";
    case FEED_FILTER_OPERATOR_CONTAINSNOT:
      return L"doesn't contain";
    default:
      return L"?";
  }
}