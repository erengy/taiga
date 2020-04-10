/*
** Taiga
** Copyright (C) 2010-2020, Eren Okka
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

#include <nstd/algorithm.hpp>

#include "track/feed_filter_manager.h"

#include "base/format.h"
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
#include "track/feed_filter_util.h"

namespace track {

FeedFilterManager::FeedFilterManager() {
  InitializePresets();
}

void FeedFilterManager::AddPresets(std::vector<FeedFilter>& filters) {
  if (!filters.empty())
    return;

  for (const auto& preset : presets_) {
    if (preset.is_default) {
      filters.push_back(preset.filter);
    }
  }
}

void FeedFilterManager::AddPresets() {
  AddPresets(filters_);
}

std::vector<FeedFilterPreset> FeedFilterManager::GetPresets() const {
  return presets_;
}

void FeedFilterManager::AddFilter(FeedFilterAction action,
                                  FeedFilterMatch match,
                                  FeedFilterOption option,
                                  bool enabled,
                                  const std::wstring& name) {
  filters_.resize(filters_.size() + 1);
  filters_.back().action = action;
  filters_.back().enabled = enabled;
  filters_.back().match = match;
  filters_.back().name = name;
  filters_.back().option = option;

  taiga::settings.SetModified();
}

std::vector<FeedFilter> FeedFilterManager::GetFilters() const {
  return filters_;
}

void FeedFilterManager::SetFilters(const std::vector<FeedFilter>& filters) {
  filters_ = filters;

  taiga::settings.SetModified();
}

void FeedFilterManager::Cleanup() {
  for (auto filter = filters_.begin(); filter != filters_.end(); ++filter) {
    auto& ids = filter->anime_ids;
    for (auto id = ids.begin(); id != ids.end(); ++id) {
      if (!anime::db.Find(*id)) {
        if (ids.size() > 1) {
          id = ids.erase(id) - 1;
          continue;
        } else {
          filter = filters_.erase(filter) - 1;
          break;
        }
        taiga::settings.SetModified();
      }
    }
  }
}

void FeedFilterManager::Filter(Feed& feed, bool preferences) {
  if (!taiga::settings.GetTorrentFilterEnabled())
    return;

  for (auto& item : feed.items) {
    for (auto& filter : filters_) {
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
      presets_.resize(presets_.size() + 1); \
      presets_.back().description = description_; \
      presets_.back().is_default = is_default_; \
      presets_.back().filter.action = action_; \
      presets_.back().filter.enabled = true; \
      presets_.back().filter.match = match_; \
      presets_.back().filter.option = option_; \
      presets_.back().filter.name = name_;
  #define ADD_CONDITION(e, o, v) \
      presets_.back().filter.AddCondition(e, o, v);

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
  ADD_CONDITION(kFeedFilterElement_User_Status, kFeedFilterOperator_Equals, ToWstr(static_cast<int>(anime::MyStatus::Watching)));

  // Select airing anime in plan to watch
  ADD_PRESET(kFeedFilterActionSelect, kFeedFilterMatchAll, true, kFeedFilterOptionDefault,
      L"Select airing anime in plan to watch",
      L"Selects files that belong to an airing anime that you're planning to watch");
  ADD_CONDITION(kFeedFilterElement_Meta_Status, kFeedFilterOperator_Equals, ToWstr(static_cast<int>(anime::SeriesStatus::Airing)));
  ADD_CONDITION(kFeedFilterElement_User_Status, kFeedFilterOperator_Equals, ToWstr(static_cast<int>(anime::MyStatus::PlanToWatch)));

  // Discard dropped
  ADD_PRESET(kFeedFilterActionDiscard, kFeedFilterMatchAll, true, kFeedFilterOptionDefault,
      L"Discard dropped",
      L"Discards files that belong to anime that you've dropped watching");
  ADD_CONDITION(kFeedFilterElement_User_Status, kFeedFilterOperator_Equals, ToWstr(static_cast<int>(anime::MyStatus::Dropped)));

  // Discard and deactivate not-in-list anime
  ADD_PRESET(kFeedFilterActionDiscard, kFeedFilterMatchAny, true, kFeedFilterOptionDeactivate,
      L"Discard and deactivate not-in-list anime",
      L"Discards files that do not belong to any anime in your list");
  ADD_CONDITION(kFeedFilterElement_User_Status, kFeedFilterOperator_Equals, ToWstr(static_cast<int>(anime::MyStatus::NotInList)));

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

    filter.action = static_cast<FeedFilterAction>(util::GetIndexFromShortcode(
        util::Shortcode::Action, item.attribute(L"action").value()));
    filter.enabled = item.attribute(L"enabled").as_bool();
    filter.match = static_cast<FeedFilterMatch>(util::GetIndexFromShortcode(
        util::Shortcode::Match, item.attribute(L"match").value()));
    filter.name = item.attribute(L"name").value();
    filter.option = static_cast<FeedFilterOption>(util::GetIndexFromShortcode(
        util::Shortcode::Option, item.attribute(L"option").value()));

    for (auto anime : item.children(L"anime")) {
      filter.anime_ids.push_back(anime.attribute(L"id").as_int());
    }

    for (auto condition : item.children(L"condition")) {
      filter.AddCondition(
          static_cast<FeedFilterElement>(util::GetIndexFromShortcode(
              util::Shortcode::Element, condition.attribute(L"element").value())),
          static_cast<FeedFilterOperator>(util::GetIndexFromShortcode(
              util::Shortcode::Operator, condition.attribute(L"operator").value())),
          condition.attribute(L"value").value());
    }

    filters.push_back(filter);
  }
}

void FeedFilterManager::Import(const pugi::xml_node& node_filter) {
  Import(node_filter, filters_);
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
    item.append_attribute(L"action") = util::GetShortcodeFromIndex(
        util::Shortcode::Action, feed_filter.action).c_str();
    item.append_attribute(L"match") = util::GetShortcodeFromIndex(
        util::Shortcode::Match, feed_filter.match).c_str();
    item.append_attribute(L"option") = util::GetShortcodeFromIndex(
        util::Shortcode::Option, feed_filter.option).c_str();
    item.append_attribute(L"enabled") = feed_filter.enabled;
    item.append_attribute(L"name") = feed_filter.name.c_str();

    for (const auto& id : feed_filter.anime_ids) {
      auto anime = item.append_child(L"anime");
      anime.append_attribute(L"id") = id;
    }

    for (const auto& condition : feed_filter.conditions) {
      auto node = item.append_child(L"condition");
      node.append_attribute(L"element") = util::GetShortcodeFromIndex(
          util::Shortcode::Element, condition.element).c_str();
      node.append_attribute(L"operator") = util::GetShortcodeFromIndex(
          util::Shortcode::Operator, condition.op).c_str();
      node.append_attribute(L"value") = condition.value.c_str();
    }
  }
}

void FeedFilterManager::Export(pugi::xml_node& node_filter) {
  Export(node_filter, filters_);
}

////////////////////////////////////////////////////////////////////////////////

bool FeedFilterManager::GetFansubFilter(int anime_id,
                                        std::vector<std::wstring>& groups) {
  bool found = false;

  for (const auto& filter : filters_) {
    if (nstd::contains(filter.anime_ids, anime_id)) {
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

bool FeedFilterManager::SetFansubFilter(int anime_id,
                                        const std::wstring& group_name,
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
  for (auto it = filters_.begin(); it != filters_.end(); ++it) {
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
        taiga::settings.SetModified();
        return true;
      } else {
        break;
      }
    } else {
      if (group_name.empty()) {
        filters_.erase(it);
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
      taiga::settings.SetModified();
      return true;
    }
  }

  if (group_name.empty())
    return false;

  auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return false;

  // Create new filter
  AddFilter(track::kFeedFilterActionPrefer, track::kFeedFilterMatchAll,
            track::kFeedFilterOptionDefault, true,
            L"[Fansub] " + GetPreferredTitle(*anime_item));
  filters_.back().AddCondition(track::kFeedFilterElement_Episode_Group,
                               track::kFeedFilterOperator_Equals, group_name);
  if (!video_resolution.empty()) {
    filters_.back().AddCondition(
        track::kFeedFilterElement_Episode_VideoResolution,
        track::kFeedFilterOperator_Equals, video_resolution);
  }
  filters_.back().anime_ids.push_back(anime_id);

  return true;
}

bool FeedFilterManager::AddDiscardFilter(int anime_id) {
  auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return false;

  AddFilter(track::kFeedFilterActionDiscard, track::kFeedFilterMatchAll,
            track::kFeedFilterOptionDefault, true,
            L"Discard \"{}\""_format(anime::GetPreferredTitle(*anime_item)));
  filters_.back().AddCondition(track::kFeedFilterElement_Meta_Id,
                               track::kFeedFilterOperator_Equals,
                               ToWstr(anime_item->GetId()));

  return true;
}

}  // namespace track
