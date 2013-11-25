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

#include "feed.h"

#include "library/anime.h"
#include "library/anime_db.h"
#include "base/common.h"
#include "base/foreach.h"
#include "sync/myanimelist.h"
#include "taiga/settings.h"
#include "base/string.h"

// =============================================================================

bool EvaluateCondition(const FeedFilterCondition& condition, const FeedItem& item) {
  bool is_numeric = false;
  wstring element, value = ReplaceVariables(condition.value, item.episode_data);
  auto anime = AnimeDatabase.FindItem(item.episode_data.anime_id);

  switch (condition.element) {
    case FEED_FILTER_ELEMENT_FILE_TITLE:
      element = item.title;
      break;
    case FEED_FILTER_ELEMENT_FILE_CATEGORY:
      element = item.category;
      break;
    case FEED_FILTER_ELEMENT_FILE_DESCRIPTION:
      element = item.description;
      break;
    case FEED_FILTER_ELEMENT_FILE_LINK:
      element = item.link;
      break;
    case FEED_FILTER_ELEMENT_META_ID:
      if (anime) element = ToWstr(anime->GetId());
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_EPISODE_TITLE:
      element = item.episode_data.title;
      break;
    case FEED_FILTER_ELEMENT_META_DATE_START:
      if (anime) element = anime->GetDate(anime::DATE_START);
      break;
    case FEED_FILTER_ELEMENT_META_DATE_END:
      if (anime) element = anime->GetDate(anime::DATE_END);
      break;
    case FEED_FILTER_ELEMENT_META_EPISODES:
      if (anime) element = ToWstr(anime->GetEpisodeCount());
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_META_STATUS:
      if (anime) element = ToWstr(anime->GetAiringStatus());
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_META_TYPE:
      if (anime) element = ToWstr(anime->GetType());
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_USER_STATUS:
      if (anime) element = ToWstr(anime->GetMyStatus());
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_EPISODE_NUMBER:
      element = ToWstr(GetEpisodeHigh(item.episode_data.number));
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_EPISODE_VERSION:
      element = item.episode_data.version;
      if (element.empty()) element = L"1";
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_LOCAL_EPISODE_AVAILABLE:
      if (anime) element = ToWstr(anime->IsEpisodeAvailable(
        GetEpisodeHigh(item.episode_data.number)));
      is_numeric = true;
      break;
    case FEED_FILTER_ELEMENT_EPISODE_GROUP:
      element = item.episode_data.group;
      break;
    case FEED_FILTER_ELEMENT_EPISODE_VIDEO_RESOLUTION:
      element = item.episode_data.resolution;
      break;
    case FEED_FILTER_ELEMENT_EPISODE_VIDEO_TYPE:
      element = item.episode_data.video_type;
      break;
  }

  switch (condition.op) {
    case FEED_FILTER_OPERATOR_EQUALS:
      if (is_numeric) {
        if (IsEqual(value, L"True")) return ToInt(element) == TRUE;
        return ToInt(element) == ToInt(value);
      } else {
        if (condition.element == FEED_FILTER_ELEMENT_EPISODE_VIDEO_RESOLUTION) {
          return TranslateResolution(element) == TranslateResolution(condition.value);
        } else {
          return IsEqual(element, value);
        }
      }
    case FEED_FILTER_OPERATOR_NOTEQUALS:
      if (is_numeric) {
        if (IsEqual(value, L"True")) return ToInt(element) == TRUE;
        return ToInt(element) != ToInt(value);
      } else {
        if (condition.element == FEED_FILTER_ELEMENT_EPISODE_VIDEO_RESOLUTION) {
          return TranslateResolution(element) != TranslateResolution(condition.value);
        } else {
          return !IsEqual(element, value);
        }
      }
    case FEED_FILTER_OPERATOR_ISGREATERTHAN:
      if (is_numeric) {
        return ToInt(element) > ToInt(value);
      } else {
        if (condition.element == FEED_FILTER_ELEMENT_EPISODE_VIDEO_RESOLUTION) {
          return TranslateResolution(element) > TranslateResolution(condition.value);
        } else {
          return CompareStrings(element, condition.value) > 0;
        }
      }
    case FEED_FILTER_OPERATOR_ISGREATERTHANOREQUALTO:
      if (is_numeric) {
        return ToInt(element) >= ToInt(value);
      } else {
        if (condition.element == FEED_FILTER_ELEMENT_EPISODE_VIDEO_RESOLUTION) {
          return TranslateResolution(element) >= TranslateResolution(condition.value);
        } else {
          return CompareStrings(element, condition.value) >= 0;
        }
      }
    case FEED_FILTER_OPERATOR_ISLESSTHAN:
      if (is_numeric) {
        return ToInt(element) < ToInt(value);
      } else {
        if (condition.element == FEED_FILTER_ELEMENT_EPISODE_VIDEO_RESOLUTION) {
          return TranslateResolution(element) < TranslateResolution(condition.value);
        } else {
          return CompareStrings(element, condition.value) < 0;
        }
      }
    case FEED_FILTER_OPERATOR_ISLESSTHANOREQUALTO:
      if (is_numeric) {
        return ToInt(element) <= ToInt(value);
      } else {
        if (condition.element == FEED_FILTER_ELEMENT_EPISODE_VIDEO_RESOLUTION) {
          return TranslateResolution(element) <= TranslateResolution(condition.value);
        } else {
          return CompareStrings(element, condition.value) <= 0;
        }
      }
    case FEED_FILTER_OPERATOR_BEGINSWITH:
      return StartsWith(element, value);
    case FEED_FILTER_OPERATOR_ENDSWITH:
      return EndsWith(element, value);
    case FEED_FILTER_OPERATOR_CONTAINS:
      return InStr(element, value, 0, true) > -1;
    case FEED_FILTER_OPERATOR_NOTCONTAINS:
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
  element = FEED_FILTER_ELEMENT_FILE_TITLE;
  op = FEED_FILTER_OPERATOR_EQUALS;
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

void FeedFilter::Filter(Feed& feed, FeedItem& item, bool recursive) {
  if (!enabled)
    return;

  // No need to filter if the item was discarded before
  if (item.state == FEEDITEM_DISCARDED)
    return;

  if (!anime_ids.empty()) {
    bool apply_filter = false;
    foreach_(id, anime_ids) {
      if (*id == item.episode_data.anime_id) {
        apply_filter = true;
        break;
      }
    }
    if (!apply_filter)
      return; // Filter doesn't apply to this item
  }

  bool matched = false;
  size_t condition_index = 0;

  switch (match) {
    case FEED_FILTER_MATCH_ALL:
      matched = true;
      for (size_t i = 0; i < conditions.size(); i++) {
        if (!EvaluateCondition(conditions[i], item)) {
          matched = false;
          condition_index = i;
          break;
        }
      }
      break;
    case FEED_FILTER_MATCH_ANY:
      matched = false;
      for (size_t i = 0; i < conditions.size(); i++) {
        if (EvaluateCondition(conditions[i], item)) {
          matched = true;
          condition_index = i;
          break;
        }
      }
      break;
  }

  switch (action) {
    case FEED_FILTER_ACTION_DISCARD:
      if (matched) {
        // Discard matched items, regardless of their previous state
        item.state = FEEDITEM_DISCARDED;
      } else {
        return; // Filter doesn't apply to this item
      }
      break;

    case FEED_FILTER_ACTION_SELECT:
      if (matched) {
        // Select matched items, if they were not discarded before
        item.state = FEEDITEM_SELECTED;
      } else {
        return; // Filter doesn't apply to this item
      }
      break;

    case FEED_FILTER_ACTION_PREFER: {
      if (recursive) {
        if (matched) {
          foreach_(it, feed.items) {
            // Do not bother if the item was discarded before
            if (it->state == FEEDITEM_DISCARDED) continue;
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
            // Is it the same group?
            if (!IsEqual(it->episode_data.group, item.episode_data.group)) continue;
            // Try applying the same filter
            Filter(feed, *it, false);
          }
        }
        // Filters are strong if they're limited, weak otherwise
        bool strong_preference = !anime_ids.empty();
        if (strong_preference) {
          if (matched) {
            // Select matched items, if they were not discarded before
            item.state = FEEDITEM_SELECTED;
          } else {
            // Discard mismatched items, regardless of their previous state
            item.state = FEEDITEM_DISCARDED;
          }
        } else {
          return; // Filter doesn't apply to this item
        }
      } else {
        // The fact that we're here means that the preference filter matched an
        // item before, and now we're checking other items for mismatches. At
        // this point, we don't care whether the preference is weak or strong.
        if (!matched) {
          // Discard mismatched items, regardless of their previous state
          item.state = FEEDITEM_DISCARDED;
        } else {
          return; // Filter doesn't apply to this item
        }
      }
      break;
    }
  }

#ifdef _DEBUG
  wstring filter_text =
      (item.state == FEEDITEM_DISCARDED ? L"!FILTER :: " : L"FILTER :: ") +
      Aggregator.filter_manager.TranslateConditions(*this, condition_index);
  item.description = filter_text + L" -- " + item.description;
#endif
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
  InitializePresets();
  InitializeShortcodes();
}

void FeedFilterManager::AddPresets() {
  foreach_(preset, presets) {
    if (!preset->is_default) continue;
    AddFilter(preset->filter.action, preset->filter.match,
              preset->filter.enabled, preset->filter.name);
    foreach_(condition, preset->filter.conditions) {
      filters.back().AddCondition(condition->element,
                                  condition->op,
                                  condition->value);
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
  foreach_(filter, filters) {
    foreach_(id, filter->anime_ids) {
      if (!AnimeDatabase.FindItem(*id)) {
        if (filter->anime_ids.size() > 1) {
          id = filter->anime_ids.erase(id) - 1;
          continue;
        } else {
          filter = filters.erase(filter) - 1;
          break;
        }
      }
    }
  }
}

void FeedFilterManager::Filter(Feed& feed, bool preferences) {
  if (!Settings.RSS.Torrent.Filters.global_enabled)
    return;

  foreach_(item, feed.items) {
    foreach_(filter, filters) {
      if (preferences != (filter->action == FEED_FILTER_ACTION_PREFER))
        continue;
      filter->Filter(feed, *item, true);
    }
  }
}

void FeedFilterManager::FilterArchived(Feed& feed) {
  foreach_(item, feed.items) {
    if (item->state != FEEDITEM_DISCARDED) {
      bool found = Aggregator.SearchArchive(item->title);
      if (found) {
        item->state = FEEDITEM_DISCARDED;
#ifdef _DEBUG
        wstring filter_text = L"!FILTER :: Archived";
        item->description = filter_text + L" -- " + item->description;
#endif
      }
    }
  }
}

bool FeedFilterManager::IsItemDownloadAvailable(Feed& feed) {
  foreach_c_(item, feed.items)
    if (item->state == FEEDITEM_SELECTED)
      return true;

  return false;
}

void FeedFilterManager::MarkNewEpisodes(Feed& feed) {
  foreach_(item, feed.items) {
    auto anime_item = AnimeDatabase.FindItem(item->episode_data.anime_id);
    if (anime_item) {
      int number = GetEpisodeHigh(item->episode_data.number);
      if (number > anime_item->GetMyLastWatchedEpisode())
        item->episode_data.new_episode = true;
    }
  }
}

// =============================================================================

void FeedFilterManager::InitializePresets() {
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
  ADD_PRESET(FEED_FILTER_ACTION_PREFER, FEED_FILTER_MATCH_ALL, false, 
      L"[Fansub] Anime", 
      L"Lets you choose a fansub group for one or more anime");
  ADD_CONDITION(FEED_FILTER_ELEMENT_EPISODE_GROUP, FEED_FILTER_OPERATOR_EQUALS, L"TaigaSubs (change this)");

  // Discard bad video keywords
  ADD_PRESET(FEED_FILTER_ACTION_DISCARD, FEED_FILTER_MATCH_ANY, false, 
      L"Discard bad video keywords", 
      L"Discards everything that is AVI, DIVX, LQ, RMVB, SD, WMV or XVID");
  ADD_CONDITION(FEED_FILTER_ELEMENT_EPISODE_VIDEO_TYPE, FEED_FILTER_OPERATOR_CONTAINS, L"AVI");
  ADD_CONDITION(FEED_FILTER_ELEMENT_EPISODE_VIDEO_TYPE, FEED_FILTER_OPERATOR_CONTAINS, L"DIVX");
  ADD_CONDITION(FEED_FILTER_ELEMENT_EPISODE_VIDEO_TYPE, FEED_FILTER_OPERATOR_CONTAINS, L"LQ");
  ADD_CONDITION(FEED_FILTER_ELEMENT_EPISODE_VIDEO_TYPE, FEED_FILTER_OPERATOR_CONTAINS, L"RMVB");
  ADD_CONDITION(FEED_FILTER_ELEMENT_EPISODE_VIDEO_TYPE, FEED_FILTER_OPERATOR_CONTAINS, L"SD");
  ADD_CONDITION(FEED_FILTER_ELEMENT_EPISODE_VIDEO_TYPE, FEED_FILTER_OPERATOR_CONTAINS, L"WMV");
  ADD_CONDITION(FEED_FILTER_ELEMENT_EPISODE_VIDEO_TYPE, FEED_FILTER_OPERATOR_CONTAINS, L"XVID");

  // Prefer new versions
  ADD_PRESET(FEED_FILTER_ACTION_PREFER, FEED_FILTER_MATCH_ANY, false, 
      L"Prefer new versions", 
      L"Prefers v2 files and above when there are earlier releases of the same episode as well");
  ADD_CONDITION(FEED_FILTER_ELEMENT_EPISODE_VERSION, FEED_FILTER_OPERATOR_ISGREATERTHAN, L"1");

  /* Default filters */

  // Select currently watching
  ADD_PRESET(FEED_FILTER_ACTION_SELECT, FEED_FILTER_MATCH_ANY, true, 
      L"Select currently watching", 
      L"Selects files that belong to anime that you're currently watching");
  ADD_CONDITION(FEED_FILTER_ELEMENT_USER_STATUS, FEED_FILTER_OPERATOR_EQUALS, ToWstr(sync::myanimelist::kWatching));

  // Discard unknown titles
  ADD_PRESET(FEED_FILTER_ACTION_DISCARD, FEED_FILTER_MATCH_ANY, true, 
      L"Discard unknown titles", 
      L"Discards files that do not belong to any anime in your list");
  ADD_CONDITION(FEED_FILTER_ELEMENT_META_ID, FEED_FILTER_OPERATOR_EQUALS, L"");

  // Discard watched and available episodes
  ADD_PRESET(FEED_FILTER_ACTION_DISCARD, FEED_FILTER_MATCH_ANY, true, 
      L"Discard watched and available episodes", 
      L"Discards episodes you've already watched or downloaded");
  ADD_CONDITION(FEED_FILTER_ELEMENT_EPISODE_NUMBER, FEED_FILTER_OPERATOR_ISLESSTHANOREQUALTO, L"%watched%");
  ADD_CONDITION(FEED_FILTER_ELEMENT_LOCAL_EPISODE_AVAILABLE, FEED_FILTER_OPERATOR_EQUALS, L"True");

  // Prefer high-resolution files
  ADD_PRESET(FEED_FILTER_ACTION_PREFER, FEED_FILTER_MATCH_ANY, true, 
      L"Prefer high-resolution files", 
      L"Prefers 720p files when there are other files of the same episode as well");
  ADD_CONDITION(FEED_FILTER_ELEMENT_EPISODE_VIDEO_RESOLUTION, FEED_FILTER_OPERATOR_EQUALS, L"720p");

  #undef ADD_CONDITION
  #undef ADD_PRESET
}

void FeedFilterManager::InitializeShortcodes() {
  action_shortcodes_[FEED_FILTER_ACTION_DISCARD] = L"discard";
  action_shortcodes_[FEED_FILTER_ACTION_SELECT] = L"select";
  action_shortcodes_[FEED_FILTER_ACTION_PREFER] = L"prefer";

  element_shortcodes_[FEED_FILTER_ELEMENT_META_ID] = L"meta_id";
  element_shortcodes_[FEED_FILTER_ELEMENT_META_STATUS] = L"meta_status";
  element_shortcodes_[FEED_FILTER_ELEMENT_META_TYPE] = L"meta_type";
  element_shortcodes_[FEED_FILTER_ELEMENT_META_EPISODES] = L"meta_episodes";
  element_shortcodes_[FEED_FILTER_ELEMENT_META_DATE_START] = L"meta_date_start";
  element_shortcodes_[FEED_FILTER_ELEMENT_META_DATE_END] = L"meta_date_end";
  element_shortcodes_[FEED_FILTER_ELEMENT_USER_STATUS] = L"user_status";
  element_shortcodes_[FEED_FILTER_ELEMENT_LOCAL_EPISODE_AVAILABLE] = L"local_episode_available";
  element_shortcodes_[FEED_FILTER_ELEMENT_EPISODE_TITLE] = L"episode_title";
  element_shortcodes_[FEED_FILTER_ELEMENT_EPISODE_NUMBER] = L"episode_number";
  element_shortcodes_[FEED_FILTER_ELEMENT_EPISODE_VERSION] = L"episode_version";
  element_shortcodes_[FEED_FILTER_ELEMENT_EPISODE_GROUP] = L"episode_group";
  element_shortcodes_[FEED_FILTER_ELEMENT_EPISODE_VIDEO_RESOLUTION] = L"episode_video_resolution";
  element_shortcodes_[FEED_FILTER_ELEMENT_EPISODE_VIDEO_TYPE] = L"episode_video_type";
  element_shortcodes_[FEED_FILTER_ELEMENT_FILE_TITLE] = L"file_title";
  element_shortcodes_[FEED_FILTER_ELEMENT_FILE_CATEGORY] = L"file_category";
  element_shortcodes_[FEED_FILTER_ELEMENT_FILE_DESCRIPTION] = L"file_description";
  element_shortcodes_[FEED_FILTER_ELEMENT_FILE_LINK] = L"file_link";

  match_shortcodes_[FEED_FILTER_MATCH_ALL] = L"all";
  match_shortcodes_[FEED_FILTER_MATCH_ANY] = L"any";

  operator_shortcodes_[FEED_FILTER_OPERATOR_EQUALS] = L"equals";
  operator_shortcodes_[FEED_FILTER_OPERATOR_NOTEQUALS] = L"notequals";
  operator_shortcodes_[FEED_FILTER_OPERATOR_ISGREATERTHAN] = L"gt";
  operator_shortcodes_[FEED_FILTER_OPERATOR_ISGREATERTHANOREQUALTO] = L"ge";
  operator_shortcodes_[FEED_FILTER_OPERATOR_ISLESSTHAN] = L"lt";
  operator_shortcodes_[FEED_FILTER_OPERATOR_ISLESSTHANOREQUALTO] = L"le";
  operator_shortcodes_[FEED_FILTER_OPERATOR_BEGINSWITH] = L"beginswith";
  operator_shortcodes_[FEED_FILTER_OPERATOR_ENDSWITH] = L"endswith";
  operator_shortcodes_[FEED_FILTER_OPERATOR_CONTAINS] = L"contains";
  operator_shortcodes_[FEED_FILTER_OPERATOR_NOTCONTAINS] = L"notcontains";
}

wstring FeedFilterManager::CreateNameFromConditions(const FeedFilter& filter) {
  // TODO
  return L"New Filter";
}

wstring FeedFilterManager::TranslateCondition(const FeedFilterCondition& condition) {
  return TranslateElement(condition.element) + L" " + 
         TranslateOperator(condition.op) + L" \"" + 
         TranslateValue(condition) + L"\"";
}

wstring FeedFilterManager::TranslateConditions(const FeedFilter& filter, size_t index) {
  wstring str;
  
  size_t max_index = (filter.match == FEED_FILTER_MATCH_ALL) ?
      filter.conditions.size() : index + 1;
  
  for (size_t i = index; i < max_index; i++) {
    if (i > index) str += L" & ";
    str += TranslateCondition(filter.conditions[i]);
  }

  return str;
}

wstring FeedFilterManager::TranslateElement(int element) {
  switch (element) {
    case FEED_FILTER_ELEMENT_FILE_TITLE:
      return L"File name";
    case FEED_FILTER_ELEMENT_FILE_CATEGORY:
      return L"File category";
    case FEED_FILTER_ELEMENT_FILE_DESCRIPTION:
      return L"File description";
    case FEED_FILTER_ELEMENT_FILE_LINK:
      return L"File link";
    case FEED_FILTER_ELEMENT_META_ID:
      return L"Anime ID";
    case FEED_FILTER_ELEMENT_EPISODE_TITLE:
      return L"Episode title";
    case FEED_FILTER_ELEMENT_META_DATE_START:
      return L"Anime date started";
    case FEED_FILTER_ELEMENT_META_DATE_END:
      return L"Anime date ended";
    case FEED_FILTER_ELEMENT_META_EPISODES:
      return L"Anime episode count";
    case FEED_FILTER_ELEMENT_META_STATUS:
      return L"Anime airing status";
    case FEED_FILTER_ELEMENT_META_TYPE:
      return L"Anime type";
    case FEED_FILTER_ELEMENT_USER_STATUS:
      return L"Anime watching status";
    case FEED_FILTER_ELEMENT_EPISODE_NUMBER:
      return L"Episode number";
    case FEED_FILTER_ELEMENT_EPISODE_VERSION:
      return L"Episode version";
    case FEED_FILTER_ELEMENT_LOCAL_EPISODE_AVAILABLE:
      return L"Episode availability";
    case FEED_FILTER_ELEMENT_EPISODE_GROUP:
      return L"Episode fansub group";
    case FEED_FILTER_ELEMENT_EPISODE_VIDEO_RESOLUTION:
      return L"Episode video resolution";
    case FEED_FILTER_ELEMENT_EPISODE_VIDEO_TYPE:
      return L"Episode video type";
    default:
      return L"?";
  }
}

wstring FeedFilterManager::TranslateOperator(int op) {
  switch (op) {
    case FEED_FILTER_OPERATOR_EQUALS:
      return L"is";
    case FEED_FILTER_OPERATOR_NOTEQUALS:
      return L"is not";
    case FEED_FILTER_OPERATOR_ISGREATERTHAN:
      return L"is greater than";
    case FEED_FILTER_OPERATOR_ISGREATERTHANOREQUALTO:
      return L"is greater than or equal to";
    case FEED_FILTER_OPERATOR_ISLESSTHAN:
      return L"is less than";
    case FEED_FILTER_OPERATOR_ISLESSTHANOREQUALTO:
      return L"is less than or equal to";
    case FEED_FILTER_OPERATOR_BEGINSWITH:
      return L"begins with";
    case FEED_FILTER_OPERATOR_ENDSWITH:
      return L"ends with";
    case FEED_FILTER_OPERATOR_CONTAINS:
      return L"contains";
    case FEED_FILTER_OPERATOR_NOTCONTAINS:
      return L"does not contain";
    default:
      return L"?";
  }
}

wstring FeedFilterManager::TranslateValue(const FeedFilterCondition& condition) {
  switch (condition.element) {
    case FEED_FILTER_ELEMENT_META_ID: {
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
    case FEED_FILTER_ELEMENT_USER_STATUS:
      return sync::myanimelist::TranslateMyStatus(ToInt(condition.value), false);
    case FEED_FILTER_ELEMENT_META_STATUS:
      return sync::myanimelist::TranslateStatus(ToInt(condition.value));
    case FEED_FILTER_ELEMENT_META_TYPE:
      return sync::myanimelist::TranslateType(ToInt(condition.value));
    default:
      return condition.value;
  }
}

wstring FeedFilterManager::TranslateMatching(int match) {
  switch (match) {
    case FEED_FILTER_MATCH_ALL:
      return L"All conditions";
    case FEED_FILTER_MATCH_ANY:
      return L"Any condition";
    default:
      return L"?";
  }
}

wstring FeedFilterManager::TranslateAction(int action) {
  switch (action) {
    case FEED_FILTER_ACTION_DISCARD:
      return L"Discard matched items";
    case FEED_FILTER_ACTION_SELECT:
      return L"Select matched items";
    case FEED_FILTER_ACTION_PREFER:
      return L"Prefer matched items to similar ones";
    default:
      return L"?";
  }
}

wstring FeedFilterManager::GetShortcodeFromIndex(FeedFilterShortcodeType type,
                                                 int index) {
  switch (type) {
    case FEED_FILTER_SHORTCODE_ACTION:
      return action_shortcodes_[index];
    case FEED_FILTER_SHORTCODE_ELEMENT:
      return element_shortcodes_[index];
    case FEED_FILTER_SHORTCODE_MATCH:
      return match_shortcodes_[index];
    case FEED_FILTER_SHORTCODE_OPERATOR:
      return operator_shortcodes_[index];
  }

  return wstring();
}

int FeedFilterManager::GetIndexFromShortcode(FeedFilterShortcodeType type,
                                             const wstring& shortcode) {
  std::map<int, wstring>* shortcodes = nullptr;
  switch (type) {
    case FEED_FILTER_SHORTCODE_ACTION:
      shortcodes = &action_shortcodes_;
      break;
    case FEED_FILTER_SHORTCODE_ELEMENT:
      shortcodes = &element_shortcodes_;
      break;
    case FEED_FILTER_SHORTCODE_MATCH:
      shortcodes = &match_shortcodes_;
      break;
    case FEED_FILTER_SHORTCODE_OPERATOR:
      shortcodes = &operator_shortcodes_;
      break;
  }

  foreach_(it, *shortcodes)
    if (IsEqual(it->second, shortcode))
      return it->first;

  return -1;
}