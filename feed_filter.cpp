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

#include "std.h"

#include "feed.h"

#include "anime.h"
#include "anime_db.h"
#include "common.h"
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

bool EvaluateCondition(const FeedFilterCondition& condition, const FeedItem& item) {
  bool is_numeric = false;
  wstring element, value = ReplaceVariables(condition.value, item.episode_data);
  auto anime = AnimeDatabase.FindItem(item.episode_data.anime_id);

  switch (condition.element) {
    case FEED_FILTER_ELEMENT_TITLE:
      element = item.title;
      break;
    case FEED_FILTER_ELEMENT_CATEGORY:
      element = item.category;
      break;
    case FEED_FILTER_ELEMENT_DESCRIPTION:
      element = item.description;
      break;
    case FEED_FILTER_ELEMENT_LINK:
      element = item.link;
      break;
    case FEED_FILTER_ELEMENT_ANIME_ID:
      if (anime) element = ToWstr(anime->GetId());
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_ANIME_TITLE:
      element = item.episode_data.title;
      break;
    case FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS:
      if (anime) element = ToWstr(anime->GetAiringStatus());
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_ANIME_MYSTATUS:
      if (anime) element = ToWstr(anime->GetMyStatus());
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_ANIME_EPISODE_NUMBER:
      element = ToWstr(GetEpisodeHigh(item.episode_data.number));
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_ANIME_EPISODE_VERSION:
      element = item.episode_data.version;
      break;
    case FEED_FILTER_ELEMENT_ANIME_EPISODE_AVAILABLE:
      if (anime) element = ToWstr(anime->IsEpisodeAvailable(
        GetEpisodeHigh(item.episode_data.number)));
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_ANIME_GROUP:
      element = item.episode_data.group;
      break;
    case FEED_FILTER_ELEMENT_ANIME_VIDEO_RESOLUTION:
      element = item.episode_data.resolution;
      break;
    case FEED_FILTER_ELEMENT_ANIME_VIDEO_TYPE:
      element = item.episode_data.video_type;
      break;
  }

  switch (condition.op) {
    case FEED_FILTER_OPERATOR_IS:
      if (is_numeric) {
        if (IsEqual(value, L"True")) return ToInt(element) == TRUE;
        return ToInt(element) == ToInt(value);
      } else {
        return IsEqual(element, value);
      }
    case FEED_FILTER_OPERATOR_ISNOT:
      if (is_numeric) {
        if (IsEqual(value, L"True")) return ToInt(element) == TRUE;
        return ToInt(element) != ToInt(value);
      } else {
        return !IsEqual(element, value);
      }
    case FEED_FILTER_OPERATOR_ISGREATERTHAN:
      if (is_numeric) {
        return ToInt(element) > ToInt(value);
      } else {
        if (condition.element == FEED_FILTER_ELEMENT_ANIME_VIDEO_RESOLUTION) {
          return TranslateResolution(element) > TranslateResolution(condition.value);
        } else {
          return CompareStrings(element, condition.value) > 0;
        }
      }
    case FEED_FILTER_OPERATOR_ISLESSTHAN:
      if (is_numeric) {
        return ToInt(element) < ToInt(value);
      } else {
        if (condition.element == FEED_FILTER_ELEMENT_ANIME_VIDEO_RESOLUTION) {
          return TranslateResolution(element) < TranslateResolution(condition.value);
        } else {
          return CompareStrings(element, condition.value) < 0;
        }
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

FeedFilterCondition& FeedFilterCondition::operator=(const FeedFilterCondition& condition) {
  element = condition.element;
  op = condition.op;
  value = condition.value;

  return *this;
}

void FeedFilterCondition::Reset() {
  element = FEED_FILTER_ELEMENT_TITLE;
  op = FEED_FILTER_OPERATOR_IS;
  value.clear();
}

// =============================================================================

FeedFilter& FeedFilter::operator=(const FeedFilter& filter) {
  action = filter.action;
  enabled = filter.enabled;
  match = filter.match;
  name = filter.name;

  conditions.resize(filter.conditions.size());
  std::copy(filter.conditions.begin(), filter.conditions.end(), conditions.begin());

  anime_ids.resize(filter.anime_ids.size());
  std::copy(filter.anime_ids.begin(), filter.anime_ids.end(), anime_ids.begin());

  return *this;
}

void FeedFilter::AddCondition(int element, int op, const wstring& value) {
  conditions.resize(conditions.size() + 1);
  conditions.back().element = element;
  conditions.back().op = op;
  conditions.back().value = value;
}

bool FeedFilter::Filter(Feed& feed, FeedItem& item, bool recursive) {
  if (!enabled) return true;

  bool condition = false;
  size_t index = 0;

  if (!anime_ids.empty()) {
    auto anime_item = AnimeDatabase.FindItem(item.episode_data.anime_id);
    if (anime_item) {
      for (auto it = anime_ids.begin(); it != anime_ids.end(); ++it) {
        if (*it == anime_item->GetId()) {
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
  
  switch (match) {
    case FEED_FILTER_MATCH_ALL:
      condition = true;
      for (size_t i = 0; i < conditions.size(); i++) {
        if (!EvaluateCondition(conditions[i], item)) {
          condition = false;
          index = i;
          break;
        }
      }
      break;
    case FEED_FILTER_MATCH_ANY:
      condition = false;
      for (size_t i = 0; i < conditions.size(); i++) {
        if (EvaluateCondition(conditions[i], item)) {
          condition = true;
          index = i;
          break;
        }
      }
      break;
  }
  
  if (EvaluateAction(action, condition)) {
    // Handle preferences
    if (action == FEED_FILTER_ACTION_PREFER && recursive) {
      for (auto it = feed.items.begin(); it != feed.items.end(); ++it) {
        // Do not bother if the item failed before
        if (it->download == false) continue;
        // Do not filter the same item again
        if (it->index == item.index) continue;
        // Is it the same title?
        if (it->episode_data.anime_id == anime::ID_NOTINLIST) {
          if (!IsEqual(it->episode_data.title, item.episode_data.title)) continue;
        } else {
          if (it->episode_data.anime_id != item.episode_data.anime_id) continue;
        }
        // Is it the same episode?
        if (it->episode_data.number != item.episode_data.number) continue;
        // Try applying the same filter
        if (!this->Filter(feed, *it, false)) {
          // Hey, we don't prefer your kind around here!
          it->download = false;
        }
      }
    }

    // Item passed the filter (tick!)
    return true;
  
  } else {
    if (action == FEED_FILTER_ACTION_PREFER && recursive) {
      return true;
    
    } else {
#ifdef _DEBUG
      item.description = L"!FILTER :: " + 
        Aggregator.filter_manager.TranslateConditions(*this, index) +
        L" -- " + item.description;
#endif

      // Out you go, you pathetic feed item!
      return false;
    }
  }
}

void FeedFilter::Reset() {
  enabled = true;
  action = FEED_FILTER_ACTION_DISCARD;
  match = FEED_FILTER_MATCH_ALL;
  anime_ids.clear();
  conditions.clear();
  name.clear();
}

// =============================================================================

FeedFilterManager::FeedFilterManager() {
  #define ADD_PRESET(action_, match_, is_default_, name_, description_) \
    presets.resize(presets.size() + 1); \
    presets.back().description = description_; \
    presets.back().is_default = is_default_; \
    presets.back().filter.action = action_; \
    presets.back().filter.enabled = true; \
    presets.back().filter.match = match_; \
    presets.back().filter.name = name_;
  #define ADD_CONDITION(e, o, v) \
    presets.back().filter.AddCondition(e, o, v);

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
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_MYSTATUS, FEED_FILTER_OPERATOR_IS, ToWstr(mal::MYSTATUS_COMPLETED));
  
  // Discard dropped titles
  ADD_PRESET(FEED_FILTER_ACTION_DISCARD, FEED_FILTER_MATCH_ALL, true, 
    L"Discard dropped titles", 
    L"Discards files that belong to anime you've dropped");
  ADD_CONDITION(FEED_FILTER_ELEMENT_ANIME_MYSTATUS, FEED_FILTER_OPERATOR_IS, ToWstr(mal::MYSTATUS_DROPPED));
  
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

void FeedFilterManager::AddPresets() {
  for (auto it = presets.begin(); it != presets.end(); ++it) {
    if (!it->is_default) continue;
    AddFilter(it->filter.action, it->filter.match, it->filter.enabled, it->filter.name);
    for (auto c = it->filter.conditions.begin(); c != it->filter.conditions.end(); ++c) {
      filters.back().AddCondition(c->element, c->op, c->value);
    }
  }
}

void FeedFilterManager::AddFilter(int action, int match, bool enabled, const wstring& name) {
  filters.resize(filters.size() + 1);
  filters.back().action = action;
  filters.back().enabled = enabled;
  filters.back().match = match;
  filters.back().name = name;
}

void FeedFilterManager::Cleanup() {
  for (auto i = filters.begin(); i != filters.end(); ++i) {
    for (auto j = i->anime_ids.begin(); j != i->anime_ids.end(); ++j) {
      if (!AnimeDatabase.FindItem(*j)) {
        if (i->anime_ids.size() > 1) {
          j = i->anime_ids.erase(j) - 1;
          continue;
        } else {
          i = filters.erase(i) - 1;
          break;
        }
      }
    }
  }
}

int FeedFilterManager::Filter(Feed& feed) {
  bool download = true;
  anime::Item* anime_item = nullptr;
  int count = 0, number = 0;
  
  for (auto item = feed.items.begin(); item != feed.items.end(); ++item) {
    download = true;
    anime_item = AnimeDatabase.FindItem(item->episode_data.anime_id);
    number = GetEpisodeHigh(item->episode_data.number);
    
    if (anime_item && number > anime_item->GetMyLastWatchedEpisode()) {
      item->episode_data.new_episode = true;
    }

    if (!item->download) continue;

    // Apply filters
    if (Settings.RSS.Torrent.Filters.global_enabled) {
      for (size_t j = 0; j < filters.size(); j++) {
        if (!filters[j].Filter(feed, *item, true)) {
          download = false;
          break;
        }
      }
    }
    
    // Check archive
    if (download) {
      download = !Aggregator.SearchArchive(item->title);
    }

    // Mark item
    if (download) {
      item->download = true;
      count++;
    } else {
      item->download = false;
    }
  }

  return count;
}

// =============================================================================

wstring FeedFilterManager::CreateNameFromConditions(const FeedFilter& filter) {
  wstring name;

  // TODO
  name = L"New Filter";

  return name;
}

wstring FeedFilterManager::TranslateCondition(const FeedFilterCondition& condition) {
  return TranslateElement(condition.element) + L" " + 
    TranslateOperator(condition.op) + L" \"" + 
    TranslateValue(condition) + L"\"";
}

wstring FeedFilterManager::TranslateConditions(const FeedFilter& filter, size_t index) {
  wstring str;
  
  size_t max_index = filter.match == FEED_FILTER_MATCH_ALL ? 
    filter.conditions.size() : index + 1;
  
  for (size_t i = index; i < max_index; i++) {
    if (i > index) str += L" & ";
    str += TranslateCondition(filter.conditions[i]);
  }

  return str;
}

wstring FeedFilterManager::TranslateElement(int element) {
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

wstring FeedFilterManager::TranslateOperator(int op) {
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

wstring FeedFilterManager::TranslateValue(const FeedFilterCondition& condition) {
  switch (condition.element) {
    case FEED_FILTER_ELEMENT_ANIME_ID: {
      if (condition.value.empty()) {
        return L"(?)";
      } else {
        auto anime_item = AnimeDatabase.FindItem(ToInt(condition.value));
        if (anime_item) {
          return condition.value + L" (" + anime_item->GetTitle() + L")";
        } else {
          return condition.value + L" (?)";
        }
      }
    }
    case FEED_FILTER_ELEMENT_ANIME_MYSTATUS:
      return mal::TranslateMyStatus(ToInt(condition.value), false);
    case FEED_FILTER_ELEMENT_ANIME_SERIESSTATUS:
      return mal::TranslateStatus(ToInt(condition.value));
    default:
      return condition.value;
  }
}

wstring FeedFilterManager::TranslateMatching(int match) {
  switch (match) {
    case FEED_FILTER_MATCH_ALL:
      return L"Match all conditions";
    case FEED_FILTER_MATCH_ANY:
      return L"Match any condition";
    default:
      return L"?";
  }
}

wstring FeedFilterManager::TranslateAction(int action) {
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