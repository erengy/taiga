/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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

#include "track/feed_filter_manager.h"

#include "base/log.h"
#include "base/string.h"
#include "base/xml.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "track/episode_util.h"
#include "track/feed.h"
#include "track/feed_aggregator.h"
#include "ui/translate.h"

namespace track {

FeedFilterManager::FeedFilterManager() {
  InitializePresets();
  InitializeShortcodes();
}

void FeedFilterManager::AddPresets() {
  for (const auto& preset : presets) {
    if (!preset.is_default)
      continue;

    AddFilter(preset.filter.action, preset.filter.match,
              preset.filter.option, preset.filter.enabled,
              preset.filter.name);

    for (const auto& condition : preset.filter.conditions) {
      filters.back().AddCondition(condition.element,
                                  condition.op,
                                  condition.value);
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
  for (auto filter = filters.begin(); filter != filters.end(); ++filter) {
    auto& ids = filter->anime_ids;
    for (auto id = ids.begin(); id != ids.end(); ++id) {
      if (!anime::db.Find(*id)) {
        if (ids.size() > 1) {
          id = ids.erase(id) - 1;
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
  if (!taiga::settings.GetTorrentFilterEnabled())
    return;

  for (auto& item : feed.items) {
    for (auto& filter : filters) {
      if (preferences != (filter.action == kFeedFilterActionPrefer))
        continue;
      filter.Filter(feed, item, true);
    }
  }
}

void FeedFilterManager::FilterArchived(Feed& feed) {
  for (auto& item : feed.items) {
    if (!item.IsDiscarded()) {
      if (aggregator.archive.Contains(item.title)) {
        item.state = FeedItemState::DiscardedNormal;
        if (Taiga.options.debug_mode) {
          std::wstring filter_text = L"!FILTER :: Archived";
          item.description = filter_text + L" -- " + item.description;
        }
      }
    }
  }
}

void FeedFilterManager::MarkNewEpisodes(Feed& feed) {
  for (auto& feed_item : feed.items) {
    auto anime_item = anime::db.Find(feed_item.episode_data.anime_id);
    if (anime_item) {
      int number = anime::GetEpisodeHigh(feed_item.episode_data);
      if (number > anime_item->GetMyLastWatchedEpisode())
        feed_item.episode_data.new_episode = true;
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

  // Discard dropped
  ADD_PRESET(kFeedFilterActionDiscard, kFeedFilterMatchAll, true, kFeedFilterOptionDefault,
      L"Discard dropped",
      L"Discards files that belong to anime that you've dropped watching");
  ADD_CONDITION(kFeedFilterElement_User_Status, kFeedFilterOperator_Equals, ToWstr(anime::kDropped));

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
  element_shortcodes_[kFeedFilterElement_File_Size] = L"file_size";

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
  XmlDocument document;
  const auto parse_result = document.load_string(input.c_str());

  if (!parse_result)
    return false;

  Import(document, filters);
  return true;
}

void FeedFilterManager::Import(const XmlNode& node_filter,
                               std::vector<FeedFilter>& filters) {
  filters.clear();

  for (auto item : node_filter.children(L"item")) {
    FeedFilter filter;

    filter.action = static_cast<FeedFilterAction>(GetIndexFromShortcode(
        kFeedFilterShortcodeAction, item.attribute(L"action").value()));
    filter.enabled = item.attribute(L"enabled").as_bool();
    filter.match = static_cast<FeedFilterMatch>(GetIndexFromShortcode(
        kFeedFilterShortcodeMatch, item.attribute(L"match").value()));
    filter.name = item.attribute(L"name").value();
    filter.option = static_cast<FeedFilterOption>(GetIndexFromShortcode(
        kFeedFilterShortcodeOption, item.attribute(L"option").value()));

    for (auto anime : item.children(L"anime")) {
      filter.anime_ids.push_back(anime.attribute(L"id").as_int());
    }

    for (auto condition : item.children(L"condition")) {
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
  XmlDocument node_filter;
  Export(node_filter, filters);
  output = XmlDump(node_filter);
}

void FeedFilterManager::Export(pugi::xml_node& node_filter,
                               const std::vector<FeedFilter>& filters) {
  for (const auto& feed_filter : filters) {
    auto item = node_filter.append_child(L"item");
    item.append_attribute(L"action") =
        feed_filter_manager.GetShortcodeFromIndex(
            kFeedFilterShortcodeAction, feed_filter.action).c_str();
    item.append_attribute(L"match") =
        feed_filter_manager.GetShortcodeFromIndex(
            kFeedFilterShortcodeMatch, feed_filter.match).c_str();
    item.append_attribute(L"option") =
        feed_filter_manager.GetShortcodeFromIndex(
            kFeedFilterShortcodeOption, feed_filter.option).c_str();
    item.append_attribute(L"enabled") = feed_filter.enabled;
    item.append_attribute(L"name") = feed_filter.name.c_str();

    for (const auto& id : feed_filter.anime_ids) {
      auto anime = item.append_child(L"anime");
      anime.append_attribute(L"id") = id;
    }

    for (const auto& condition : feed_filter.conditions) {
      auto node = item.append_child(L"condition");
      node.append_attribute(L"element") =
          feed_filter_manager.GetShortcodeFromIndex(
              kFeedFilterShortcodeElement, condition.element).c_str();
      node.append_attribute(L"operator") =
          feed_filter_manager.GetShortcodeFromIndex(
              kFeedFilterShortcodeOperator, condition.op).c_str();
      node.append_attribute(L"value") = condition.value.c_str();
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
    case kFeedFilterElement_File_Size:
      return L"File size";
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
        auto anime_item = anime::db.Find(ToInt(condition.value));
        if (anime_item) {
          return condition.value + L" (" + anime::GetPreferredTitle(*anime_item) + L")";
        } else {
          return condition.value + L" (?)";
        }
      }
    }
    case kFeedFilterElement_User_Status:
      return ui::TranslateMyStatus(ToInt(condition.value), false);
    case kFeedFilterElement_Meta_Status:
      return ui::TranslateStatus(ToInt(condition.value));
    case kFeedFilterElement_Meta_Type:
      return ui::TranslateType(static_cast<anime::SeriesType>(ToInt(condition.value)));
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

  for (const auto& pair : *shortcodes) {
    if (IsEqual(pair.second, shortcode))
      return pair.first;
  }

  LOGD(L"Shortcode: \"{}\" for type \"{}\" is not found.", shortcode, type);

  return -1;
}

////////////////////////////////////////////////////////////////////////////////

bool GetFansubFilter(int anime_id, std::vector<std::wstring>& groups) {
  bool found = false;

  for (const auto& filter : feed_filter_manager.filters) {
    if (std::find(filter.anime_ids.begin(), filter.anime_ids.end(),
                  anime_id) != filter.anime_ids.end()) {
      for (const auto& condition : filter.conditions) {
        if (condition.element == track::kFeedFilterElement_Episode_Group) {
          groups.push_back(condition.value);
          found = true;
        }
      }
    }
  }

  return found;
}

bool SetFansubFilter(int anime_id, const std::wstring& group_name,
                     const std::wstring& video_resolution) {
  const auto find_condition = [](track::FeedFilter& filter,
                                 track::FeedFilterElement element) {
    return std::find_if(
        filter.conditions.begin(), filter.conditions.end(),
        [&element](const track::FeedFilterCondition& condition) {
          return condition.element == element;
        });
  };

  // Check existing filters
  auto& filters = feed_filter_manager.filters;
  for (auto it = filters.begin(); it != filters.end(); ++it) {
    auto& filter = *it;
    auto id = std::find(
        filter.anime_ids.begin(), filter.anime_ids.end(), anime_id);
    if (id == filter.anime_ids.end())
      continue;

    const auto group_condition =
        find_condition(filter, track::kFeedFilterElement_Episode_Group);
    if (group_condition == filter.conditions.end())
      continue;

    const auto resolution_condition =
        find_condition(filter, track::kFeedFilterElement_Episode_VideoResolution);

    if (filter.anime_ids.size() > 1) {
      filter.anime_ids.erase(id);
      if (group_name.empty()) {
        return true;
      } else {
        break;
      }
    } else {
      if (group_name.empty()) {
        filters.erase(it);
      } else {
        group_condition->value = group_name;
        if (!video_resolution.empty()) {
          if (resolution_condition != filter.conditions.end()) {
            resolution_condition->value = video_resolution;
          } else {
            filter.AddCondition(track::kFeedFilterElement_Episode_VideoResolution,
                                track::kFeedFilterOperator_Equals, video_resolution);
          }
        }
      }
      return true;
    }
  }

  if (group_name.empty())
    return false;

  auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return false;

  // Create new filter
  feed_filter_manager.AddFilter(
      track::kFeedFilterActionPrefer, track::kFeedFilterMatchAll, track::kFeedFilterOptionDefault,
      true, L"[Fansub] " + GetPreferredTitle(*anime_item));
  feed_filter_manager.filters.back().AddCondition(
      track::kFeedFilterElement_Episode_Group, track::kFeedFilterOperator_Equals,
      group_name);
  if (!video_resolution.empty()) {
    feed_filter_manager.filters.back().AddCondition(
        track::kFeedFilterElement_Episode_VideoResolution, track::kFeedFilterOperator_Equals,
        video_resolution);
  }
  feed_filter_manager.filters.back().anime_ids.push_back(anime_id);
  return true;
}

}  // namespace track
