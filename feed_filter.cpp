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
#include "settings.h"
#include "string.h"

// =============================================================================

bool EvaluateAction(int action, bool condition) {
  switch (action) {
    case FEED_FILTER_ACTION_DISCARD:
      return !condition;
    case FEED_FILTER_ACTION_SELECT:
      return condition;
    case FEED_FILTER_ACTION_PREFER:
      return condition;
    default:
      return true;
  }
}

bool EvaluateCondition(const CFeedFilterCondition& condition, const CFeedItem& item) {
  bool is_numeric = false;
  wstring element, value = ReplaceVariables(condition.Value, item.EpisodeData);
  CAnime* anime = item.EpisodeData.Index > 0 ? 
    &AnimeList.Item.at(item.EpisodeData.Index) : nullptr;

  switch (condition.Element) {
    case FEED_FILTER_ELEMENT_TITLE:
      element = item.Title;
      break;
    case FEED_FILTER_ELEMENT_CATEGORY:
      element = item.Category;
      break;
    case FEED_FILTER_ELEMENT_DESCRIPTION:
      element = item.Description;
      break;
    case FEED_FILTER_ELEMENT_LINK:
      element = item.Link;
      break;
    case FEED_FILTER_ELEMENT_ANIME_ID:
      if (anime) element = ToWSTR(anime->Series_ID);
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_ANIME_TITLE:
      element = item.EpisodeData.Title;
      break;
    case FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS:
      if (anime) element = ToWSTR(anime->GetAiringStatus());
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_ANIME_MYSTATUS:
      if (anime) element = ToWSTR(anime->GetStatus());
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_ANIME_EPISODE_NUMBER:
      element = ToWSTR(GetEpisodeHigh(item.EpisodeData.Number));
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_ANIME_EPISODE_VERSION:
      element = item.EpisodeData.Version;
      break;
    case FEED_FILTER_ELEMENT_ANIME_EPISODE_AVAILABLE:
      if (anime) element = ToWSTR(anime->IsEpisodeAvailable(
        GetEpisodeHigh(item.EpisodeData.Number)));
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_ANIME_GROUP:
      element = item.EpisodeData.Group;
      break;
    case FEED_FILTER_ELEMENT_ANIME_VIDEO_RESOLUTION:
      element = item.EpisodeData.Resolution;
      break;
    case FEED_FILTER_ELEMENT_ANIME_VIDEO_TYPE:
      element = item.EpisodeData.VideoType;
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

CFeedFilterCondition& CFeedFilterCondition::operator=(const CFeedFilterCondition& condition) {
  Element = condition.Element;
  Operator = condition.Operator;
  Value = condition.Value;

  return *this;
}

void CFeedFilterCondition::Reset() {
  Element = FEED_FILTER_ELEMENT_TITLE;
  Operator = FEED_FILTER_OPERATOR_IS;
  Value.clear();
}

// =============================================================================

CFeedFilter& CFeedFilter::operator=(const CFeedFilter& filter) {
  Action = filter.Action;
  Enabled = filter.Enabled;
  Match = filter.Match;
  Name = filter.Name;

  Conditions.resize(filter.Conditions.size());
  std::copy(filter.Conditions.begin(), filter.Conditions.end(), Conditions.begin());

  AnimeID.resize(filter.AnimeID.size());
  std::copy(filter.AnimeID.begin(), filter.AnimeID.end(), AnimeID.begin());

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

  if (!AnimeID.empty()) {
    if (item.EpisodeData.Index > 0 && item.EpisodeData.Index < static_cast<int>(AnimeList.Item.size())) {
      for (auto it = AnimeID.begin(); it != AnimeID.end(); ++it) {
        if (*it == AnimeList.Item.at(item.EpisodeData.Index).Series_ID) {
          condition = true;
          break;
        }
      }
    }
    if (!condition) {
      // Filter doesn't apply to this item
      return true;
    }
  }
  
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
    if (Action == FEED_FILTER_ACTION_PREFER && recursive) {
      return true;
    
    } else {
      #ifdef _DEBUG
      item.Description = L"!FILTER :: " + 
        Aggregator.FilterManager.TranslateConditions(*this, index) +
        L" -- " + item.Description;
      #endif

      // Out you go, you pathetic feed item!
      return false;
    }
  }
}

void CFeedFilter::Reset() {
  Enabled = true;
  Action = FEED_FILTER_ACTION_DISCARD;
  Match = FEED_FILTER_MATCH_ALL;
  AnimeID.clear();
  Conditions.clear();
  Name.clear();
}

// =============================================================================

CFeedFilterManager::CFeedFilterManager() {
  #define ADD_PRESET(action, match, def, name, desc) \
    Presets.resize(Presets.size() + 1); \
    Presets.back().Default = def; \
    Presets.back().Description = desc; \
    Presets.back().Filter.Action = action; \
    Presets.back().Filter.Enabled = true; \
    Presets.back().Filter.Match = match; \
    Presets.back().Filter.Name = name;
  #define ADD_CONDITION(e, o, v) \
    Presets.back().Filter.AddCondition(e, o, v);

  /* Preset filters */

  // Custom
  ADD_PRESET(FEED_FILTER_ACTION_DISCARD, FEED_FILTER_MATCH_ALL, false, 
    L"(Custom)", 
    L"Lets you create a custom filter from scratch");

  // Fansub group
  ADD_PRESET(FEED_FILTER_ACTION_SELECT, FEED_FILTER_MATCH_ANY, false, 
    L"[Fansub] Anime", 
    L"Lets you choose a fansub group for one or more anime");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_GROUP, FEED_FILTER_OPERATOR_IS, L"TaigaSubs (change this)");

  // Discard video keywords
  ADD_PRESET(FEED_FILTER_ACTION_DISCARD, FEED_FILTER_MATCH_ANY, false, 
    L"Discard bad video keywords", 
    L"Discards everything that is AVI, DIVX, LQ, RMVB, SD, WMV or XVID");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_VIDEO_TYPE, FEED_FILTER_OPERATOR_CONTAINS, L"AVI");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_VIDEO_TYPE, FEED_FILTER_OPERATOR_CONTAINS, L"DIVX");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_VIDEO_TYPE, FEED_FILTER_OPERATOR_CONTAINS, L"LQ");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_VIDEO_TYPE, FEED_FILTER_OPERATOR_CONTAINS, L"RMVB");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_VIDEO_TYPE, FEED_FILTER_OPERATOR_CONTAINS, L"SD");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_VIDEO_TYPE, FEED_FILTER_OPERATOR_CONTAINS, L"WMV");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_VIDEO_TYPE, FEED_FILTER_OPERATOR_CONTAINS, L"XVID");

  // Prefer new versions
  ADD_PRESET(FEED_FILTER_ACTION_PREFER, FEED_FILTER_MATCH_ANY, false, 
    L"Prefer new versions", 
    L"Prefers v2 files when there are earlier releases of the same episode as well");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_EPISODE_VERSION, FEED_FILTER_OPERATOR_IS, L"v2");

  /* Default filters */

  // Discard unknown titles
  ADD_PRESET(FEED_FILTER_ACTION_DISCARD, FEED_FILTER_MATCH_ALL, true, 
    L"Discard unknown titles", 
    L"Discards files that do not belong to any anime in your list");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_ID, FEED_FILTER_OPERATOR_IS, L"");
  
  // Discard completed titles
  ADD_PRESET(FEED_FILTER_ACTION_DISCARD, FEED_FILTER_MATCH_ALL, true, 
    L"Discard completed titles", 
    L"Discards files that belong to anime you've already finished");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_MYSTATUS, FEED_FILTER_OPERATOR_IS, ToWSTR(MAL_COMPLETED));
  
  // Discard dropped titles
  ADD_PRESET(FEED_FILTER_ACTION_DISCARD, FEED_FILTER_MATCH_ALL, true, 
    L"Discard dropped titles", 
    L"Discards files that belong to anime you've dropped");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_MYSTATUS, FEED_FILTER_OPERATOR_IS, ToWSTR(MAL_DROPPED));
  
  // Select new episodes only
  ADD_PRESET(FEED_FILTER_ACTION_SELECT, FEED_FILTER_MATCH_ALL, true, 
    L"Select new episodes only", 
    L"Discards episodes you've already watched or downloaded");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_EPISODE_NUMBER, FEED_FILTER_OPERATOR_ISGREATERTHAN, L"%watched%");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_EPISODE_AVAILABLE, FEED_FILTER_OPERATOR_IS, L"False");
  
  // Prefer high resolution files
  ADD_PRESET(FEED_FILTER_ACTION_PREFER, FEED_FILTER_MATCH_ANY, true, 
    L"Prefer high-resolution files", 
    L"Prefers 720p files when there are other files of the same episode as well");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_VIDEO_RESOLUTION, FEED_FILTER_OPERATOR_CONTAINS, L"720p");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_VIDEO_RESOLUTION, FEED_FILTER_OPERATOR_CONTAINS, L"1280x720");

  #undef ADD_CONDITION
  #undef ADD_PRESET
}

void CFeedFilterManager::AddPresets() {
  for (auto it = Presets.begin(); it != Presets.end(); ++it) {
    if (!it->Default) continue;
    AddFilter(it->Filter.Action, it->Filter.Match, it->Filter.Enabled, it->Filter.Name);
    for (auto c = it->Filter.Conditions.begin(); c != it->Filter.Conditions.end(); ++c) {
      Filters.back().AddCondition(c->Element, c->Operator, c->Value);
    }
  }
}

void CFeedFilterManager::AddFilter(int action, int match, bool enabled, const wstring& name) {
  Filters.resize(Filters.size() + 1);
  Filters.back().Action = action;
  Filters.back().Enabled = enabled;
  Filters.back().Match = match;
  Filters.back().Name = name;
}

void CFeedFilterManager::Cleanup() {
  for (auto i = Filters.begin(); i != Filters.end(); ++i) {
    for (auto j = i->AnimeID.begin(); j != i->AnimeID.end(); ++j) {
      CAnime* anime = AnimeList.FindItem(*j);
      if (!anime) {
        if (i->AnimeID.size() > 1) {
          j = i->AnimeID.erase(j) - 1;
          continue;
        } else {
          i = Filters.erase(i) - 1;
          break;
        }
      }
    }
  }
}

int CFeedFilterManager::Filter(CFeed& feed) {
  bool download = true;
  CAnime* anime = nullptr;
  int count = 0, number = 0;
  
  for (auto item = feed.Item.begin(); item != feed.Item.end(); ++item) {
    download = true;
    anime = item->EpisodeData.Index > 0 ? &AnimeList.Item[item->EpisodeData.Index] : nullptr;
    number = GetEpisodeHigh(item->EpisodeData.Number);
    
    if (anime > 0 && number > anime->GetLastWatchedEpisode()) {
      item->EpisodeData.NewEpisode = true;
    }

    if (!item->Download) continue;

    // Apply filters
    if (Settings.RSS.Torrent.Filters.GlobalEnabled) {
      for (size_t j = 0; j < Filters.size(); j++) {
        if (!Filters[j].Filter(feed, *item, true)) {
          download = false;
          break;
        }
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

wstring CFeedFilterManager::CreateNameFromConditions(const CFeedFilter& filter) {
  wstring name;

  // TODO
  name = L"New Filter";

  return name;
}

wstring CFeedFilterManager::TranslateCondition(const CFeedFilterCondition& condition) {
  return TranslateElement(condition.Element) + L" " + 
    TranslateOperator(condition.Operator) + L" \"" + 
    TranslateValue(condition) + L"\"";
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
      return L"File name";
    case FEED_FILTER_ELEMENT_CATEGORY:
      return L"File category";
    case FEED_FILTER_ELEMENT_DESCRIPTION:
      return L"File description";
    case FEED_FILTER_ELEMENT_LINK:
      return L"File link";
    case FEED_FILTER_ELEMENT_ANIME_ID:
      return L"Anime ID";
    case FEED_FILTER_ELEMENT_ANIME_TITLE:
      return L"Anime title";
    case FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS:
      return L"Anime airing status";
    case FEED_FILTER_ELEMENT_ANIME_MYSTATUS:
      return L"Anime watching status";
    case FEED_FILTER_ELEMENT_ANIME_EPISODE_NUMBER:
      return L"Episode number";
    case FEED_FILTER_ELEMENT_ANIME_EPISODE_VERSION:
      return L"Episode version";
    case FEED_FILTER_ELEMENT_ANIME_EPISODE_AVAILABLE:
      return L"Episode availability";
    case FEED_FILTER_ELEMENT_ANIME_GROUP:
      return L"Fansub group";
    case FEED_FILTER_ELEMENT_ANIME_VIDEO_RESOLUTION:
      return L"Video resolution";
    case FEED_FILTER_ELEMENT_ANIME_VIDEO_TYPE:
      return L"Video type";
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

wstring CFeedFilterManager::TranslateValue(const CFeedFilterCondition& condition) {
  switch (condition.Element) {
    case FEED_FILTER_ELEMENT_ANIME_ID: {
      if (condition.Value.empty()) {
        return L"(?)";
      } else {
        CAnime* anime = AnimeList.FindItem(ToINT(condition.Value));
        if (anime) {
          return condition.Value + L" (" + anime->Series_Title + L")";
        } else {
          return condition.Value + L" (?)";
        }
      }
    }
    case FEED_FILTER_ELEMENT_ANIME_MYSTATUS:
      return MAL.TranslateMyStatus(ToINT(condition.Value), false);
    case FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS:
      return MAL.TranslateStatus(ToINT(condition.Value));
    default:
      return condition.Value;
  }
}

wstring CFeedFilterManager::TranslateMatching(int match) {
  switch (match) {
    case FEED_FILTER_MATCH_ALL:
      return L"Match all conditions";
    case FEED_FILTER_MATCH_ANY:
      return L"Match any condition";
    default:
      return L"?";
  }
}

wstring CFeedFilterManager::TranslateAction(int action) {
  switch (action) {
    case FEED_FILTER_ACTION_DISCARD:
      return L"Discard matched items";
    case FEED_FILTER_ACTION_SELECT:
      return L"Select matched items only";
    case FEED_FILTER_ACTION_PREFER:
      return L"Prefer matched items to similar ones";
    default:
      return L"?";
  }
}