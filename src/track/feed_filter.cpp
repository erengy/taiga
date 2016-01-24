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

#include "base/foreach.h"
#include "base/log.h"
#include "base/string.h"
#include "base/xml.h"
#include "library/anime.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "taiga/script.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "track/feed.h"
#include "track/feed_filter.h"

bool EvaluateCondition(const FeedFilterCondition& condition,
                       const FeedItem& item) {
  bool is_numeric = false;
  std::wstring element;
  std::wstring value = ReplaceVariables(condition.value, item.episode_data);
  auto anime = AnimeDatabase.FindItem(item.episode_data.anime_id);

  switch (condition.element) {
    case kFeedFilterElement_File_Title:
      element = item.title;
      break;
    case kFeedFilterElement_File_Category:
      element = item.category;
      break;
    case kFeedFilterElement_File_Description:
      element = item.description;
      break;
    case kFeedFilterElement_File_Link:
      element = item.link;
      break;
    case kFeedFilterElement_Meta_Id:
      if (anime)
        element = ToWstr(anime->GetId());
      is_numeric = true;
      break;
    case kFeedFilterElement_Episode_Title:
      element = item.episode_data.anime_title();
      break;
    case kFeedFilterElement_Meta_DateStart:
      if (anime)
        element = anime->GetDateStart();
      break;
    case kFeedFilterElement_Meta_DateEnd:
      if (anime)
        element = anime->GetDateEnd();
      break;
    case kFeedFilterElement_Meta_Episodes:
      if (anime)
        element = ToWstr(anime->GetEpisodeCount());
      is_numeric = true;
      break;
    case kFeedFilterElement_Meta_Status:
      if (anime)
        element = ToWstr(anime->GetAiringStatus());
      is_numeric = true;
      break;
    case kFeedFilterElement_Meta_Type:
      if (anime)
        element = ToWstr(anime->GetType());
      is_numeric = true;
      break;
    case kFeedFilterElement_User_Status:
      element = ToWstr(anime ? anime->GetMyStatus() : anime::kNotInList);
      is_numeric = true;
      break;
    case kFeedFilterElement_User_Tags:
      if (anime)
        element = anime->GetMyTags();
      break;
    case kFeedFilterElement_Episode_Number:
      if (!item.episode_data.episode_number()) {
        element = ToWstr(anime ? anime->GetEpisodeCount() : 1);
      } else {
        element = ToWstr(anime::GetEpisodeHigh(item.episode_data));
      }
      is_numeric = true;
      break;
    case kFeedFilterElement_Episode_Version:
      element = ToWstr(item.episode_data.release_version());  // defaults to 1
      is_numeric = true;
      break;
    case kFeedFilterElement_Local_EpisodeAvailable:
      if (anime)
        element = ToWstr(anime->IsEpisodeAvailable(
            anime::GetEpisodeHigh(item.episode_data)));
      is_numeric = true;
      break;
    case kFeedFilterElement_Episode_Group:
      element = item.episode_data.release_group();
      break;
    case kFeedFilterElement_Episode_VideoResolution:
      element = item.episode_data.video_resolution();
      break;
    case kFeedFilterElement_Episode_VideoType:
      element = item.episode_data.video_terms();
      break;
  }

  switch (condition.op) {
    case kFeedFilterOperator_Equals:
      if (is_numeric) {
        if (IsEqual(value, L"True"))
          return ToInt(element) == TRUE;
        return ToInt(element) == ToInt(value);
      } else {
        if (condition.element == kFeedFilterElement_Episode_VideoResolution) {
          return anime::TranslateResolution(element) == anime::TranslateResolution(condition.value);
        } else {
          return IsEqual(element, value);
        }
      }
    case kFeedFilterOperator_NotEquals:
      if (is_numeric) {
        if (IsEqual(value, L"True"))
          return ToInt(element) == TRUE;
        return ToInt(element) != ToInt(value);
      } else {
        if (condition.element == kFeedFilterElement_Episode_VideoResolution) {
          return anime::TranslateResolution(element) != anime::TranslateResolution(condition.value);
        } else {
          return !IsEqual(element, value);
        }
      }
    case kFeedFilterOperator_IsGreaterThan:
      if (is_numeric) {
        return ToInt(element) > ToInt(value);
      } else {
        if (condition.element == kFeedFilterElement_Episode_VideoResolution) {
          return anime::TranslateResolution(element) > anime::TranslateResolution(condition.value);
        } else {
          return CompareStrings(element, condition.value) > 0;
        }
      }
    case kFeedFilterOperator_IsGreaterThanOrEqualTo:
      if (is_numeric) {
        return ToInt(element) >= ToInt(value);
      } else {
        if (condition.element == kFeedFilterElement_Episode_VideoResolution) {
          return anime::TranslateResolution(element) >= anime::TranslateResolution(condition.value);
        } else {
          return CompareStrings(element, condition.value) >= 0;
        }
      }
    case kFeedFilterOperator_IsLessThan:
      if (is_numeric) {
        return ToInt(element) < ToInt(value);
      } else {
        if (condition.element == kFeedFilterElement_Episode_VideoResolution) {
          return anime::TranslateResolution(element) < anime::TranslateResolution(condition.value);
        } else {
          return CompareStrings(element, condition.value) < 0;
        }
      }
    case kFeedFilterOperator_IsLessThanOrEqualTo:
      if (is_numeric) {
        return ToInt(element) <= ToInt(value);
      } else {
        if (condition.element == kFeedFilterElement_Episode_VideoResolution) {
          return anime::TranslateResolution(element) <= anime::TranslateResolution(condition.value);
        } else {
          return CompareStrings(element, condition.value) <= 0;
        }
      }
    case kFeedFilterOperator_BeginsWith:
      return StartsWith(element, value);
    case kFeedFilterOperator_EndsWith:
      return EndsWith(element, value);
    case kFeedFilterOperator_Contains:
      return InStr(element, value, 0, true) > -1;
    case kFeedFilterOperator_NotContains:
      return InStr(element, value, 0, true) == -1;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

FeedFilterCondition::FeedFilterCondition()
    : element(kFeedFilterElement_Meta_Id),
      op(kFeedFilterOperator_Equals) {
}

FeedFilterCondition& FeedFilterCondition::operator=(const FeedFilterCondition& condition) {
  element = condition.element;
  op = condition.op;
  value = condition.value;

  return *this;
}

void FeedFilterCondition::Reset() {
  element = kFeedFilterElement_File_Title;
  op = kFeedFilterOperator_Equals;
  value.clear();
}

////////////////////////////////////////////////////////////////////////////////

FeedFilter::FeedFilter()
    : action(kFeedFilterActionDiscard),
      enabled(true),
      match(kFeedFilterMatchAll),
      option(kFeedFilterOptionDefault) {
}

FeedFilter& FeedFilter::operator=(const FeedFilter& filter) {
  action = filter.action;
  enabled = filter.enabled;
  match = filter.match;
  name = filter.name;
  option = filter.option;

  conditions.resize(filter.conditions.size());
  std::copy(filter.conditions.begin(), filter.conditions.end(),
            conditions.begin());

  anime_ids.resize(filter.anime_ids.size());
  std::copy(filter.anime_ids.begin(), filter.anime_ids.end(),
            anime_ids.begin());

  return *this;
}

void FeedFilter::AddCondition(FeedFilterElement element,
                              FeedFilterOperator op,
                              const std::wstring& value) {
  conditions.resize(conditions.size() + 1);
  conditions.back().element = element;
  conditions.back().op = op;
  conditions.back().value = value;
}

bool FeedFilter::Filter(Feed& feed, FeedItem& item, bool recursive) {
  if (!enabled)
    return false;

  // No need to filter if the item was discarded before
  if (item.IsDiscarded())
    return false;

  if (!anime_ids.empty()) {
    bool apply_filter = false;
    foreach_(id, anime_ids) {
      if (*id == item.episode_data.anime_id) {
        apply_filter = true;
        break;
      }
    }
    if (!apply_filter)
      return false;  // Filter doesn't apply to this item
  }

  bool matched = false;
  size_t condition_index = 0;  // Used only for debugging purposes

  switch (match) {
    case kFeedFilterMatchAll:
      matched = true;
      for (size_t i = 0; i < conditions.size(); i++) {
        if (!EvaluateCondition(conditions.at(i), item)) {
          matched = false;
          condition_index = i;
          break;
        }
      }
      break;
    case kFeedFilterMatchAny:
      matched = false;
      for (size_t i = 0; i < conditions.size(); i++) {
        if (EvaluateCondition(conditions.at(i), item)) {
          matched = true;
          condition_index = i;
          break;
        }
      }
      break;
  }

  switch (action) {
    case kFeedFilterActionDiscard:
      if (matched) {
        // Discard matched items, regardless of their previous state
        item.Discard(option);
      } else {
        return false;  // Filter doesn't apply to this item
      }
      break;

    case kFeedFilterActionSelect:
      if (matched) {
        // Select matched items, if they were not discarded before
        item.state = kFeedItemSelected;
      } else {
        return false;  // Filter doesn't apply to this item
      }
      break;

    case kFeedFilterActionPrefer: {
      if (recursive) {
        // Filters are strong if they're limited, weak otherwise
        bool strong_preference = !anime_ids.empty();
        if (strong_preference) {
          if (matched) {
            // Select matched items, if they were not discarded before
            item.state = kFeedItemSelected;
          } else {
            // Discard mismatched items, regardless of their previous state
            item.Discard(option);
          }
        } else {
          if (matched) {  
            if (!ApplyPreferenceFilter(feed, item))
              return false;  // Filter didn't have any effect
          } else {
            return false;  // Filter doesn't apply to this item
          }
        }
      } else {
        // The fact that we're here means that the preference filter matched an
        // item before, and now we're checking other items for mismatches. At
        // this point, we don't care whether the preference is weak or strong.
        if (!matched) {
          // Discard mismatched items, regardless of their previous state
          item.Discard(option);
        } else {
          return false;  // Filter doesn't apply to this item
        }
      }
      break;
    }
  }

  if (Taiga.debug_mode) {
    std::wstring filter_text =
        (item.IsDiscarded() ? L"!FILTER :: " : L"FILTER :: ") +
        Aggregator.filter_manager.TranslateConditions(*this, condition_index);
    item.description = filter_text + L" -- " + item.description;
  }

  return true;
}

bool FeedFilter::ApplyPreferenceFilter(Feed& feed, FeedItem& item) {
  std::map<FeedFilterElement, bool> element_found;

  foreach_(condition, conditions) {
    switch (condition->element) {
      case kFeedFilterElement_Meta_Id:
      case kFeedFilterElement_Episode_Title:
      case kFeedFilterElement_Episode_Number:
      case kFeedFilterElement_Episode_Group:
        element_found[condition->element] = true;
        break;
    }
  }

  bool filter_applied = false;

  foreach_(it, feed.items) {
    // Do not bother if the item was discarded before
    if (it->IsDiscarded())
      continue;
    // Do not filter the same item again
    if (*it == item)
      continue;

    // Is it the same title/anime?
    if (!anime::IsValidId(it->episode_data.anime_id) &&
        !anime::IsValidId(item.episode_data.anime_id)) {
      if (!element_found[kFeedFilterElement_Episode_Title])
        if (!IsEqual(it->episode_data.anime_title(), item.episode_data.anime_title()))
          continue;
    } else {
      if (!element_found[kFeedFilterElement_Meta_Id])
        if (it->episode_data.anime_id != item.episode_data.anime_id)
          continue;
    }
    // Is it the same episode?
    if (!element_found[kFeedFilterElement_Episode_Number])
      if (it->episode_data.episode_number_range() != item.episode_data.episode_number_range())
        continue;
    // Is it from the same fansub group?
    if (!element_found[kFeedFilterElement_Episode_Group])
      if (!IsEqual(it->episode_data.release_group(), item.episode_data.release_group()))
        continue;

    // Try applying the same filter
    bool result = Filter(feed, *it, false);
    filter_applied = filter_applied || result;
  }

  return filter_applied;
}

void FeedFilter::Reset() {
  enabled = true;
  action = kFeedFilterActionDiscard;
  match = kFeedFilterMatchAll;
  anime_ids.clear();
  conditions.clear();
  name.clear();
}

////////////////////////////////////////////////////////////////////////////////

FeedFilterPreset::FeedFilterPreset()
    : is_default(false) {
}

FeedFilterManager::FeedFilterManager() {
  InitializePresets();
  InitializeShortcodes();
}

void FeedFilterManager::AddPresets() {
  foreach_(preset, presets) {
    if (!preset->is_default)
      continue;

    AddFilter(preset->filter.action, preset->filter.match,
              preset->filter.option, preset->filter.enabled,
              preset->filter.name);

    foreach_(condition, preset->filter.conditions) {
      filters.back().AddCondition(condition->element,
                                  condition->op,
                                  condition->value);
    }
  }
}

void FeedFilterManager::AddFilter(FeedFilterAction action,
                                  FeedFilterMatch match,
                                  FeedFilterOption option,
                                  bool enabled,
                                  const std::wstring& name) {
  filters.resize(filters.size() + 1);
  filters.back().action = action;
  filters.back().enabled = enabled;
  filters.back().match = match;
  filters.back().name = name;
  filters.back().option = option;
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
  if (!Settings.GetBool(taiga::kTorrent_Filter_Enabled))
    return;

  foreach_(item, feed.items) {
    foreach_(filter, filters) {
      if (preferences != (filter->action == kFeedFilterActionPrefer))
        continue;
      filter->Filter(feed, *item, true);
    }
  }
}

void FeedFilterManager::FilterArchived(Feed& feed) {
  foreach_(item, feed.items) {
    if (!item->IsDiscarded()) {
      bool found = Aggregator.SearchArchive(item->title);
      if (found) {
        item->state = kFeedItemDiscardedNormal;
        if (Taiga.debug_mode) {
          std::wstring filter_text = L"!FILTER :: Archived";
          item->description = filter_text + L" -- " + item->description;
        }
      }
    }
  }
}

void FeedFilterManager::MarkNewEpisodes(Feed& feed) {
  foreach_(item, feed.items) {
    auto anime_item = AnimeDatabase.FindItem(item->episode_data.anime_id);
    if (anime_item) {
      int number = anime::GetEpisodeHigh(item->episode_data);
      if (number > anime_item->GetMyLastWatchedEpisode())
        item->episode_data.new_episode = true;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void FeedFilterManager::InitializePresets() {
  #define ADD_PRESET(action_, match_, is_default_, option_, name_, description_) \
      presets.resize(presets.size() + 1); \
      presets.back().description = description_; \
      presets.back().is_default = is_default_; \
      presets.back().filter.action = action_; \
      presets.back().filter.enabled = true; \
      presets.back().filter.match = match_; \
      presets.back().filter.option = option_; \
      presets.back().filter.name = name_;
  #define ADD_CONDITION(e, o, v) \
      presets.back().filter.AddCondition(e, o, v);

  /* Preset filters */

  // Custom
  ADD_PRESET(kFeedFilterActionDiscard, kFeedFilterMatchAll, false, kFeedFilterOptionDefault,
      L"(Custom)",
      L"Lets you create a custom filter from scratch");

  // Fansub group
  ADD_PRESET(kFeedFilterActionPrefer, kFeedFilterMatchAll, false, kFeedFilterOptionDefault,
      L"[Fansub] Anime",
      L"Lets you choose a fansub group for one or more anime");
  ADD_CONDITION(kFeedFilterElement_Episode_Group, kFeedFilterOperator_Equals, L"TaigaSubs (change this)");

  // Discard bad video keywords
  ADD_PRESET(kFeedFilterActionDiscard, kFeedFilterMatchAny, false, kFeedFilterOptionDefault,
      L"Discard bad video keywords",
      L"Discards everything that is AVI, DIVX, LQ, RMVB, SD, WMV or XVID");
  ADD_CONDITION(kFeedFilterElement_Episode_VideoType, kFeedFilterOperator_Contains, L"AVI");
  ADD_CONDITION(kFeedFilterElement_Episode_VideoType, kFeedFilterOperator_Contains, L"DIVX");
  ADD_CONDITION(kFeedFilterElement_Episode_VideoType, kFeedFilterOperator_Contains, L"LQ");
  ADD_CONDITION(kFeedFilterElement_Episode_VideoType, kFeedFilterOperator_Contains, L"RMVB");
  ADD_CONDITION(kFeedFilterElement_Episode_VideoType, kFeedFilterOperator_Contains, L"SD");
  ADD_CONDITION(kFeedFilterElement_Episode_VideoType, kFeedFilterOperator_Contains, L"WMV");
  ADD_CONDITION(kFeedFilterElement_Episode_VideoType, kFeedFilterOperator_Contains, L"XVID");

  // Prefer new versions
  ADD_PRESET(kFeedFilterActionPrefer, kFeedFilterMatchAny, false, kFeedFilterOptionDefault,
      L"Prefer new versions",
      L"Prefers v2 files and above when there are earlier releases of the same episode as well");
  ADD_CONDITION(kFeedFilterElement_Episode_Version, kFeedFilterOperator_IsGreaterThan, L"1");

  /* Default filters */

  // Select currently watching
  ADD_PRESET(kFeedFilterActionSelect, kFeedFilterMatchAny, true, kFeedFilterOptionDefault,
      L"Select currently watching",
      L"Selects files that belong to anime that you're currently watching");
  ADD_CONDITION(kFeedFilterElement_User_Status, kFeedFilterOperator_Equals, ToWstr(anime::kWatching));

  // Select airing anime in plan to watch
  ADD_PRESET(kFeedFilterActionSelect, kFeedFilterMatchAll, true, kFeedFilterOptionDefault,
      L"Select airing anime in plan to watch",
      L"Selects files that belong to an airing anime that you're planning to watch");
  ADD_CONDITION(kFeedFilterElement_Meta_Status, kFeedFilterOperator_Equals, ToWstr(anime::kAiring));
  ADD_CONDITION(kFeedFilterElement_User_Status, kFeedFilterOperator_Equals, ToWstr(anime::kPlanToWatch));

  // Discard and deactivate not-in-list anime
  ADD_PRESET(kFeedFilterActionDiscard, kFeedFilterMatchAny, true, kFeedFilterOptionDeactivate,
      L"Discard and deactivate not-in-list anime",
      L"Discards files that do not belong to any anime in your list");
  ADD_CONDITION(kFeedFilterElement_User_Status, kFeedFilterOperator_Equals, ToWstr(anime::kNotInList));

  // Discard watched and available episodes
  ADD_PRESET(kFeedFilterActionDiscard, kFeedFilterMatchAny, true, kFeedFilterOptionDefault,
      L"Discard watched and available episodes",
      L"Discards episodes you've already watched or downloaded");
  ADD_CONDITION(kFeedFilterElement_Episode_Number, kFeedFilterOperator_IsLessThanOrEqualTo, L"%watched%");
  ADD_CONDITION(kFeedFilterElement_Local_EpisodeAvailable, kFeedFilterOperator_Equals, L"True");

  // Prefer high-resolution files
  ADD_PRESET(kFeedFilterActionPrefer, kFeedFilterMatchAny, true, kFeedFilterOptionDefault,
      L"Prefer high-resolution files",
      L"Prefers 720p files when there are other files of the same episode as well");
  ADD_CONDITION(kFeedFilterElement_Episode_VideoResolution, kFeedFilterOperator_Equals, L"720p");

  #undef ADD_CONDITION
  #undef ADD_PRESET
}

void FeedFilterManager::InitializeShortcodes() {
  action_shortcodes_[kFeedFilterActionDiscard] = L"discard";
  action_shortcodes_[kFeedFilterActionSelect] = L"select";
  action_shortcodes_[kFeedFilterActionPrefer] = L"prefer";

  element_shortcodes_[kFeedFilterElement_Meta_Id] = L"meta_id";
  element_shortcodes_[kFeedFilterElement_Meta_Status] = L"meta_status";
  element_shortcodes_[kFeedFilterElement_Meta_Type] = L"meta_type";
  element_shortcodes_[kFeedFilterElement_Meta_Episodes] = L"meta_episodes";
  element_shortcodes_[kFeedFilterElement_Meta_DateStart] = L"meta_date_start";
  element_shortcodes_[kFeedFilterElement_Meta_DateEnd] = L"meta_date_end";
  element_shortcodes_[kFeedFilterElement_User_Status] = L"user_status";
  element_shortcodes_[kFeedFilterElement_User_Tags] = L"user_tags";
  element_shortcodes_[kFeedFilterElement_Local_EpisodeAvailable] = L"local_episode_available";
  element_shortcodes_[kFeedFilterElement_Episode_Title] = L"episode_title";
  element_shortcodes_[kFeedFilterElement_Episode_Number] = L"episode_number";
  element_shortcodes_[kFeedFilterElement_Episode_Version] = L"episode_version";
  element_shortcodes_[kFeedFilterElement_Episode_Group] = L"episode_group";
  element_shortcodes_[kFeedFilterElement_Episode_VideoResolution] = L"episode_video_resolution";
  element_shortcodes_[kFeedFilterElement_Episode_VideoType] = L"episode_video_type";
  element_shortcodes_[kFeedFilterElement_File_Title] = L"file_title";
  element_shortcodes_[kFeedFilterElement_File_Category] = L"file_category";
  element_shortcodes_[kFeedFilterElement_File_Description] = L"file_description";
  element_shortcodes_[kFeedFilterElement_File_Link] = L"file_link";

  match_shortcodes_[kFeedFilterMatchAll] = L"all";
  match_shortcodes_[kFeedFilterMatchAny] = L"any";

  operator_shortcodes_[kFeedFilterOperator_Equals] = L"equals";
  operator_shortcodes_[kFeedFilterOperator_NotEquals] = L"notequals";
  operator_shortcodes_[kFeedFilterOperator_IsGreaterThan] = L"gt";
  operator_shortcodes_[kFeedFilterOperator_IsGreaterThanOrEqualTo] = L"ge";
  operator_shortcodes_[kFeedFilterOperator_IsLessThan] = L"lt";
  operator_shortcodes_[kFeedFilterOperator_IsLessThanOrEqualTo] = L"le";
  operator_shortcodes_[kFeedFilterOperator_BeginsWith] = L"beginswith";
  operator_shortcodes_[kFeedFilterOperator_EndsWith] = L"endswith";
  operator_shortcodes_[kFeedFilterOperator_Contains] = L"contains";
  operator_shortcodes_[kFeedFilterOperator_NotContains] = L"notcontains";

  option_shortcodes_[kFeedFilterOptionDefault] = L"default";
  option_shortcodes_[kFeedFilterOptionDeactivate] = L"deactivate";
  option_shortcodes_[kFeedFilterOptionHide] = L"hide";
}

bool FeedFilterManager::Import(const std::wstring& input,
                               std::vector<FeedFilter>& filters) {
  xml_document document;
  xml_parse_result result = document.load(input.c_str());

  if (result.status != pugi::status_ok)
    return false;

  Import(document, filters);
  return true;
}

void FeedFilterManager::Import(const xml_node& node_filter,
                               std::vector<FeedFilter>& filters) {
  filters.clear();

  foreach_xmlnode_(item, node_filter, L"item") {
    FeedFilter filter;

    filter.action = static_cast<FeedFilterAction>(GetIndexFromShortcode(
        kFeedFilterShortcodeAction, item.attribute(L"action").value()));
    filter.enabled = item.attribute(L"enabled").as_bool();
    filter.match = static_cast<FeedFilterMatch>(GetIndexFromShortcode(
        kFeedFilterShortcodeMatch, item.attribute(L"match").value()));
    filter.name = item.attribute(L"name").value();
    filter.option = static_cast<FeedFilterOption>(GetIndexFromShortcode(
        kFeedFilterShortcodeOption, item.attribute(L"option").value()));

    foreach_xmlnode_(anime, item, L"anime") {
      filter.anime_ids.push_back(anime.attribute(L"id").as_int());
    }

    foreach_xmlnode_(condition, item, L"condition") {
      filter.AddCondition(
          static_cast<FeedFilterElement>(GetIndexFromShortcode(
              kFeedFilterShortcodeElement, condition.attribute(L"element").value())),
          static_cast<FeedFilterOperator>(GetIndexFromShortcode(
              kFeedFilterShortcodeOperator, condition.attribute(L"operator").value())),
          condition.attribute(L"value").value());
    }

    filters.push_back(filter);
  }
}

void FeedFilterManager::Export(std::wstring& output,
                               const std::vector<FeedFilter>& filters) {
  xml_document node_filter;
  Export(node_filter, filters);
  output = XmlGetNodeAsString(node_filter);
}

void FeedFilterManager::Export(pugi::xml_node& node_filter,
                               const std::vector<FeedFilter>& filters) {
  foreach_(it, filters) {
    xml_node item = node_filter.append_child(L"item");
    item.append_attribute(L"action") =
        Aggregator.filter_manager.GetShortcodeFromIndex(
            kFeedFilterShortcodeAction, it->action).c_str();
    item.append_attribute(L"match") =
        Aggregator.filter_manager.GetShortcodeFromIndex(
            kFeedFilterShortcodeMatch, it->match).c_str();
    item.append_attribute(L"option") =
        Aggregator.filter_manager.GetShortcodeFromIndex(
            kFeedFilterShortcodeOption, it->option).c_str();
    item.append_attribute(L"enabled") = it->enabled;
    item.append_attribute(L"name") = it->name.c_str();

    foreach_(ita, it->anime_ids) {
      xml_node anime = item.append_child(L"anime");
      anime.append_attribute(L"id") = *ita;
    }

    foreach_(itc, it->conditions) {
      xml_node condition = item.append_child(L"condition");
      condition.append_attribute(L"element") =
          Aggregator.filter_manager.GetShortcodeFromIndex(
              kFeedFilterShortcodeElement, itc->element).c_str();
      condition.append_attribute(L"operator") =
          Aggregator.filter_manager.GetShortcodeFromIndex(
              kFeedFilterShortcodeOperator, itc->op).c_str();
      condition.append_attribute(L"value") = itc->value.c_str();
    }
  }
}

std::wstring FeedFilterManager::CreateNameFromConditions(
    const FeedFilter& filter) {
  // TODO
  return L"New Filter";
}

std::wstring FeedFilterManager::TranslateCondition(
    const FeedFilterCondition& condition) {
  return TranslateElement(condition.element) + L" " +
         TranslateOperator(condition.op) + L" \"" +
         TranslateValue(condition) + L"\"";
}

std::wstring FeedFilterManager::TranslateConditions(const FeedFilter& filter,
                                                    size_t index) {
  std::wstring str;

  size_t max_index = (filter.match == kFeedFilterMatchAll) ?
      filter.conditions.size() : index + 1;

  for (size_t i = index; i < max_index; i++) {
    if (i > index)
      str += L" & ";
    str += TranslateCondition(filter.conditions[i]);
  }

  return str;
}

std::wstring FeedFilterManager::TranslateElement(int element) {
  switch (element) {
    case kFeedFilterElement_File_Title:
      return L"File name";
    case kFeedFilterElement_File_Category:
      return L"File category";
    case kFeedFilterElement_File_Description:
      return L"File description";
    case kFeedFilterElement_File_Link:
      return L"File link";
    case kFeedFilterElement_Meta_Id:
      return L"Anime ID";
    case kFeedFilterElement_Episode_Title:
      return L"Episode title";
    case kFeedFilterElement_Meta_DateStart:
      return L"Anime date started";
    case kFeedFilterElement_Meta_DateEnd:
      return L"Anime date ended";
    case kFeedFilterElement_Meta_Episodes:
      return L"Anime episode count";
    case kFeedFilterElement_Meta_Status:
      return L"Anime airing status";
    case kFeedFilterElement_Meta_Type:
      return L"Anime type";
    case kFeedFilterElement_User_Status:
      return L"Anime watching status";
    case kFeedFilterElement_User_Tags:
      return L"Anime tags";
    case kFeedFilterElement_Episode_Number:
      return L"Episode number";
    case kFeedFilterElement_Episode_Version:
      return L"Episode version";
    case kFeedFilterElement_Local_EpisodeAvailable:
      return L"Episode availability";
    case kFeedFilterElement_Episode_Group:
      return L"Episode fansub group";
    case kFeedFilterElement_Episode_VideoResolution:
      return L"Episode video resolution";
    case kFeedFilterElement_Episode_VideoType:
      return L"Episode video type";
    default:
      return L"?";
  }
}

std::wstring FeedFilterManager::TranslateOperator(int op) {
  switch (op) {
    case kFeedFilterOperator_Equals:
      return L"is";
    case kFeedFilterOperator_NotEquals:
      return L"is not";
    case kFeedFilterOperator_IsGreaterThan:
      return L"is greater than";
    case kFeedFilterOperator_IsGreaterThanOrEqualTo:
      return L"is greater than or equal to";
    case kFeedFilterOperator_IsLessThan:
      return L"is less than";
    case kFeedFilterOperator_IsLessThanOrEqualTo:
      return L"is less than or equal to";
    case kFeedFilterOperator_BeginsWith:
      return L"begins with";
    case kFeedFilterOperator_EndsWith:
      return L"ends with";
    case kFeedFilterOperator_Contains:
      return L"contains";
    case kFeedFilterOperator_NotContains:
      return L"does not contain";
    default:
      return L"?";
  }
}

std::wstring FeedFilterManager::TranslateValue(
    const FeedFilterCondition& condition) {
  switch (condition.element) {
    case kFeedFilterElement_Meta_Id: {
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
    case kFeedFilterElement_User_Status:
      return anime::TranslateMyStatus(ToInt(condition.value), false);
    case kFeedFilterElement_Meta_Status:
      return anime::TranslateStatus(ToInt(condition.value));
    case kFeedFilterElement_Meta_Type:
      return anime::TranslateType(ToInt(condition.value));
    default:
      if (condition.value.empty()) {
        return L"(empty)";
      } else {
        return condition.value;
      }
  }
}

std::wstring FeedFilterManager::TranslateMatching(int match) {
  switch (match) {
    case kFeedFilterMatchAll:
      return L"All conditions";
    case kFeedFilterMatchAny:
      return L"Any condition";
    default:
      return L"?";
  }
}

std::wstring FeedFilterManager::TranslateAction(int action) {
  switch (action) {
    case kFeedFilterActionDiscard:
      return L"Discard matched items";
    case kFeedFilterActionSelect:
      return L"Select matched items";
    case kFeedFilterActionPrefer:
      return L"Prefer matched items to similar ones";
    default:
      return L"?";
  }
}

std::wstring FeedFilterManager::TranslateOption(int option) {
  switch (option) {
    case kFeedFilterOptionDefault:
      return L"Default";
    case kFeedFilterOptionDeactivate:
      return L"Deactivate discarded items";
    case kFeedFilterOptionHide:
      return L"Hide discarded items";
    default:
      return L"?";
  }
}

std::wstring FeedFilterManager::GetShortcodeFromIndex(
    FeedFilterShortcodeType type, int index) {
  switch (type) {
    case kFeedFilterShortcodeAction:
      return action_shortcodes_[index];
    case kFeedFilterShortcodeElement:
      return element_shortcodes_[index];
    case kFeedFilterShortcodeMatch:
      return match_shortcodes_[index];
    case kFeedFilterShortcodeOperator:
      return operator_shortcodes_[index];
    case kFeedFilterShortcodeOption:
      return option_shortcodes_[index];
  }

  return std::wstring();
}

int FeedFilterManager::GetIndexFromShortcode(FeedFilterShortcodeType type,
                                             const std::wstring& shortcode) {
  std::map<int, std::wstring>* shortcodes = nullptr;
  switch (type) {
    case kFeedFilterShortcodeAction:
      shortcodes = &action_shortcodes_;
      break;
    case kFeedFilterShortcodeElement:
      shortcodes = &element_shortcodes_;
      break;
    case kFeedFilterShortcodeMatch:
      shortcodes = &match_shortcodes_;
      break;
    case kFeedFilterShortcodeOperator:
      shortcodes = &operator_shortcodes_;
      break;
    case kFeedFilterShortcodeOption:
      shortcodes = &option_shortcodes_;
      break;
  }

  foreach_(it, *shortcodes)
    if (IsEqual(it->second, shortcode))
      return it->first;

  LOG(LevelDebug, L"Shortcode: \"" + shortcode +
                  L"\" for type \"" + ToWstr(type) + L"\" is not found.");

  return -1;
}