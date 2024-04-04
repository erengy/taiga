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

#include <nstd/algorithm.hpp>

#include "track/feed_filter.h"

#include "base/file.h"
#include "base/format.h"
#include "base/string.h"
#include "media/anime.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "taiga/app.h"
#include "taiga/script.h"
#include "track/episode_util.h"
#include "track/feed.h"
#include "track/feed_filter_manager.h"
#include "track/feed_filter_util.h"

namespace track {

template <typename T>
bool ApplyFilterOperator(const T& a, const T& b, const FeedFilterOperator op) {
  switch (op) {
    default:
    case kFeedFilterOperator_Equals: return a == b;
    case kFeedFilterOperator_NotEquals: return a != b;
    case kFeedFilterOperator_IsGreaterThan: return a > b;
    case kFeedFilterOperator_IsGreaterThanOrEqualTo: return a >= b;
    case kFeedFilterOperator_IsLessThan: return a < b;
    case kFeedFilterOperator_IsLessThanOrEqualTo: return a <= b;
  }
}

static bool EvaluateCondition(const FeedFilterCondition& condition,
                              const FeedItem& item) {
  const auto element = [&condition, &item]() -> std::wstring {
    const auto anime = anime::db.Find(item.episode_data.anime_id);

    switch (condition.element) {
      case kFeedFilterElement_File_Title:
        return item.title;
      case kFeedFilterElement_File_Category:
        return TranslateTorrentCategory(item.torrent_category);
      case kFeedFilterElement_File_Description:
        return item.description;
      case kFeedFilterElement_File_Link:
        return item.link;
      case kFeedFilterElement_File_Size:
        return ToWstr(item.file_size);
      case kFeedFilterElement_Meta_Id:
        return ToWstr(anime ? anime->GetId() : anime::ID_UNKNOWN);
      case kFeedFilterElement_Episode_Title:
        return item.episode_data.anime_title();
      case kFeedFilterElement_Meta_DateStart:
        return anime ? anime->GetDateStart().to_string() : std::wstring{};
      case kFeedFilterElement_Meta_DateEnd:
        return anime ? anime->GetDateEnd().to_string() : std::wstring{};
      case kFeedFilterElement_Meta_Episodes:
        return anime ? ToWstr(anime->GetEpisodeCount()) : std::wstring{};
      case kFeedFilterElement_Meta_Status:
        return ToWstr(static_cast<int>(anime ? anime->GetAiringStatus()
                                             : anime::SeriesStatus::Unknown));
      case kFeedFilterElement_Meta_Type:
        return ToWstr(static_cast<int>(anime ? anime->GetType()
                                             : anime::SeriesType::Unknown));
      case kFeedFilterElement_User_Notes:
        return anime ? anime->GetMyNotes() : std::wstring{};
      case kFeedFilterElement_User_Status:
        return ToWstr(static_cast<int>(anime ? anime->GetMyStatus()
                                             : anime::MyStatus::NotInList));
      case kFeedFilterElement_Episode_Number:
        if (!item.episode_data.episode_number()) {
          return anime ? ToWstr(anime->GetEpisodeCount()) : std::wstring{};
        } else {
          return ToWstr(anime::GetEpisodeHigh(item.episode_data));
        }
      case kFeedFilterElement_Episode_Version:
        return ToWstr(item.episode_data.release_version());  // defaults to 1
      case kFeedFilterElement_Local_EpisodeAvailable:
        return anime ? ToWstr(anime->IsEpisodeAvailable(
                           anime::GetEpisodeHigh(item.episode_data)))
                     : std::wstring{};
      case kFeedFilterElement_Episode_Group:
        return item.episode_data.release_group();
      case kFeedFilterElement_Episode_VideoResolution:
        return item.episode_data.video_resolution();
      case kFeedFilterElement_Episode_VideoType:
        return item.episode_data.video_terms();
      default:
        return std::wstring{};
    }
  }();

  const auto value = ReplaceVariables(condition.value, item.episode_data);

  const auto is_numeric = [&condition, &element, &value]() {
    if (element.empty() || value.empty())
      return false;  // see issue #639
    switch (condition.element) {
      case kFeedFilterElement_File_Size:
      case kFeedFilterElement_Meta_Id:
      case kFeedFilterElement_Meta_Episodes:
      case kFeedFilterElement_Meta_Status:
      case kFeedFilterElement_Meta_Type:;
      case kFeedFilterElement_User_Status:
      case kFeedFilterElement_Episode_Number:
      case kFeedFilterElement_Episode_Version:
      case kFeedFilterElement_Local_EpisodeAvailable:
        return true;
      default:
        return false;
    }
  };

  switch (condition.op) {
    case kFeedFilterOperator_Equals:
    case kFeedFilterOperator_NotEquals:
    case kFeedFilterOperator_IsGreaterThan:
    case kFeedFilterOperator_IsGreaterThanOrEqualTo:
    case kFeedFilterOperator_IsLessThan:
    case kFeedFilterOperator_IsLessThanOrEqualTo:
      switch (condition.element) {
        case kFeedFilterElement_File_Size:
          return ApplyFilterOperator(ToUint64(element), ParseSizeString(value),
                                     condition.op);
        case kFeedFilterElement_Episode_VideoResolution:
          return ApplyFilterOperator(
              anime::GetVideoResolutionHeight(element),
              anime::GetVideoResolutionHeight(value), condition.op);
      }
      if (is_numeric()) {
        if (condition.op == kFeedFilterOperator_Equals ||
            condition.op == kFeedFilterOperator_NotEquals) {
          if (IsEqual(value, L"True")) {
            return ApplyFilterOperator(ToInt(element), TRUE, condition.op);
          }
        }
        return ApplyFilterOperator(ToInt(element), ToInt(value), condition.op);
      } else {
        if (condition.op == kFeedFilterOperator_Equals ||
            condition.op == kFeedFilterOperator_NotEquals) {
          return ApplyFilterOperator(IsEqual(element, value), true,
                                     condition.op);
        }
        return ApplyFilterOperator(CompareStrings(element, value), 0,
                                   condition.op);
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

bool ApplyFilter(const FeedFilter& filter, Feed& feed, FeedItem& item,
                 bool recursive) {
  if (!filter.enabled)
    return false;

  // No need to filter if the item was discarded before
  if (item.IsDiscarded())
    return false;

  if (!filter.anime_ids.empty()) {
    if (!nstd::contains(filter.anime_ids, item.episode_data.anime_id)) {
      return false;  // Filter doesn't apply to this item
    }
  }

  bool matched = false;
  size_t condition_index = 0;  // Used only for debugging purposes

  switch (filter.match) {
    case kFeedFilterMatchAll:
      matched = true;
      for (size_t i = 0; i < filter.conditions.size(); i++) {
        if (!EvaluateCondition(filter.conditions.at(i), item)) {
          matched = false;
          condition_index = i;
          break;
        }
      }
      break;
    case kFeedFilterMatchAny:
      matched = false;
      for (size_t i = 0; i < filter.conditions.size(); i++) {
        if (EvaluateCondition(filter.conditions.at(i), item)) {
          matched = true;
          condition_index = i;
          break;
        }
      }
      break;
  }

  switch (filter.action) {
    case kFeedFilterActionDiscard:
      if (matched) {
        // Discard matched items, regardless of their previous state
        item.Discard(filter.option);
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
        if (const bool strong_preference = !filter.anime_ids.empty()) {
          if (matched) {
            // Select matched items, if they were not discarded before
            item.state = FeedItemState::Selected;
          } else {
            // Discard mismatched items, regardless of their previous state
            item.Discard(filter.option);
          }
        } else {
          if (matched) {
            if (!ApplyPreferenceFilter(filter, feed, item))
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
          item.Discard(filter.option);
        } else {
          return false;  // Filter doesn't apply to this item
        }
      }
      break;
    }
  }

  if (taiga::app.options.debug_mode) {
    item.description = L"[{}] {} -- {}"_format(
        item.IsDiscarded() ? L"\u274c" : L"\u2713",
        util::TranslateConditions(filter, condition_index), item.description);
  }

  return true;
}

bool ApplyPreferenceFilter(const FeedFilter& filter, Feed& feed,
                           FeedItem& item) {
  std::map<FeedFilterElement, bool> element_found;

  for (const auto& condition : filter.conditions) {
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
    const bool result = ApplyFilter(filter, feed, feed_item, false);
    filter_applied = filter_applied || result;
  }

  return filter_applied;
}

}  // namespace track
