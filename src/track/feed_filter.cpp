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

#include "track/feed_filter.h"

#include "base/file.h"
#include "base/string.h"
#include "media/anime.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "taiga/script.h"
#include "taiga/taiga.h"
#include "track/episode_util.h"
#include "track/feed.h"
#include "track/feed_filter_manager.h"

namespace track {

bool EvaluateCondition(const FeedFilterCondition& condition,
                       const FeedItem& item) {
  bool is_numeric = false;
  std::wstring element;
  std::wstring value = ReplaceVariables(condition.value, item.episode_data);
  auto anime = anime::db.Find(item.episode_data.anime_id);

  switch (condition.element) {
    case kFeedFilterElement_File_Title:
      element = item.title;
      break;
    case kFeedFilterElement_File_Category:
      element = TranslateTorrentCategory(item.torrent_category);
      break;
    case kFeedFilterElement_File_Description:
      element = item.description;
      break;
    case kFeedFilterElement_File_Link:
      element = item.link;
      break;
    case kFeedFilterElement_File_Size:
      element = ToWstr(item.file_size);
      is_numeric = true;
      break;
    case kFeedFilterElement_Meta_Id:
      element = ToWstr(anime ? anime->GetId() : anime::ID_UNKNOWN);
      is_numeric = true;
      break;
    case kFeedFilterElement_Episode_Title:
      element = item.episode_data.anime_title();
      break;
    case kFeedFilterElement_Meta_DateStart:
      if (anime)
        element = anime->GetDateStart().to_string();
      break;
    case kFeedFilterElement_Meta_DateEnd:
      if (anime)
        element = anime->GetDateEnd().to_string();
      break;
    case kFeedFilterElement_Meta_Episodes:
      if (anime)
        element = ToWstr(anime->GetEpisodeCount());
      is_numeric = true;
      break;
    case kFeedFilterElement_Meta_Status:
      element = ToWstr(static_cast<int>(anime ? anime->GetAiringStatus() : anime::SeriesStatus::Unknown));
      is_numeric = true;
      break;
    case kFeedFilterElement_Meta_Type:
      element = ToWstr(static_cast<int>(anime ? anime->GetType() : anime::SeriesType::Unknown));
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
        element = anime ? ToWstr(anime->GetEpisodeCount()) : L"";
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

  if (element.empty() || value.empty())
    is_numeric = false;

  switch (condition.op) {
    case kFeedFilterOperator_Equals:
      if (is_numeric) {
        if (condition.element == kFeedFilterElement_File_Size)
          return ToUint64(element) == ParseSizeString(value);
        if (IsEqual(value, L"True"))
          return ToInt(element) == TRUE;
        return ToInt(element) == ToInt(value);
      } else {
        if (condition.element == kFeedFilterElement_Episode_VideoResolution) {
          return anime::GetVideoResolutionHeight(element) == anime::GetVideoResolutionHeight(condition.value);
        } else {
          return IsEqual(element, value);
        }
      }
    case kFeedFilterOperator_NotEquals:
      if (is_numeric) {
        if (condition.element == kFeedFilterElement_File_Size)
          return ToUint64(element) != ParseSizeString(value);
        if (IsEqual(value, L"True"))
          return ToInt(element) == TRUE;
        return ToInt(element) != ToInt(value);
      } else {
        if (condition.element == kFeedFilterElement_Episode_VideoResolution) {
          return anime::GetVideoResolutionHeight(element) != anime::GetVideoResolutionHeight(condition.value);
        } else {
          return !IsEqual(element, value);
        }
      }
    case kFeedFilterOperator_IsGreaterThan:
      if (is_numeric) {
        if (condition.element == kFeedFilterElement_File_Size)
          return ToUint64(element) > ParseSizeString(value);
        return ToInt(element) > ToInt(value);
      } else {
        if (condition.element == kFeedFilterElement_Episode_VideoResolution) {
          return anime::GetVideoResolutionHeight(element) > anime::GetVideoResolutionHeight(condition.value);
        } else {
          return CompareStrings(element, condition.value) > 0;
        }
      }
    case kFeedFilterOperator_IsGreaterThanOrEqualTo:
      if (is_numeric) {
        if (condition.element == kFeedFilterElement_File_Size)
          return ToUint64(element) >= ParseSizeString(value);
        return ToInt(element) >= ToInt(value);
      } else {
        if (condition.element == kFeedFilterElement_Episode_VideoResolution) {
          return anime::GetVideoResolutionHeight(element) >= anime::GetVideoResolutionHeight(condition.value);
        } else {
          return CompareStrings(element, condition.value) >= 0;
        }
      }
    case kFeedFilterOperator_IsLessThan:
      if (is_numeric) {
        if (condition.element == kFeedFilterElement_File_Size)
          return ToUint64(element) < ParseSizeString(value);
        return ToInt(element) < ToInt(value);
      } else {
        if (condition.element == kFeedFilterElement_Episode_VideoResolution) {
          return anime::GetVideoResolutionHeight(element) < anime::GetVideoResolutionHeight(condition.value);
        } else {
          return CompareStrings(element, condition.value) < 0;
        }
      }
    case kFeedFilterOperator_IsLessThanOrEqualTo:
      if (is_numeric) {
        if (condition.element == kFeedFilterElement_File_Size)
          return ToUint64(element) <= ParseSizeString(value);
        return ToInt(element) <= ToInt(value);
      } else {
        if (condition.element == kFeedFilterElement_Episode_VideoResolution) {
          return anime::GetVideoResolutionHeight(element) <= anime::GetVideoResolutionHeight(condition.value);
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
    for (const auto& id : anime_ids) {
      if (id == item.episode_data.anime_id) {
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
        item.state = FeedItemState::Selected;
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
            item.state = FeedItemState::Selected;
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

  if (Taiga.options.debug_mode) {
    std::wstring filter_text =
        (item.IsDiscarded() ? L"!FILTER :: " : L"FILTER :: ") +
        feed_filter_manager.TranslateConditions(*this, condition_index);
    item.description = filter_text + L" -- " + item.description;
  }

  return true;
}

bool FeedFilter::ApplyPreferenceFilter(Feed& feed, FeedItem& item) {
  std::map<FeedFilterElement, bool> element_found;

  for (const auto& condition : conditions) {
    switch (condition.element) {
      case kFeedFilterElement_Meta_Id:
      case kFeedFilterElement_Episode_Title:
      case kFeedFilterElement_Episode_Number:
      case kFeedFilterElement_Episode_Group:
        element_found[condition.element] = true;
        break;
    }
  }

  bool filter_applied = false;

  for (auto& feed_item : feed.items) {
    // Do not bother if the item was discarded before
    if (feed_item.IsDiscarded())
      continue;
    // Do not filter the same item again
    if (feed_item == item)
      continue;

    // Is it the same title/anime?
    if (!anime::IsValidId(feed_item.episode_data.anime_id) &&
        !anime::IsValidId(item.episode_data.anime_id)) {
      if (!element_found[kFeedFilterElement_Episode_Title])
        if (!IsEqual(feed_item.episode_data.anime_title(), item.episode_data.anime_title()))
          continue;
    } else {
      if (!element_found[kFeedFilterElement_Meta_Id])
        if (feed_item.episode_data.anime_id != item.episode_data.anime_id)
          continue;
    }
    // Is it the same episode?
    if (!element_found[kFeedFilterElement_Episode_Number])
      if (feed_item.episode_data.episode_number_range() != item.episode_data.episode_number_range())
        continue;
    // Is it from the same fansub group?
    if (!element_found[kFeedFilterElement_Episode_Group])
      if (!IsEqual(feed_item.episode_data.release_group(), item.episode_data.release_group()))
        continue;

    // Try applying the same filter
    bool result = Filter(feed, feed_item, false);
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

}  // namespace track
