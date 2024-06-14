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

#pragma once

#include <string>
#include <vector>

namespace track {

enum FeedFilterElement {
  kFeedFilterElement_None = -1,
  kFeedFilterElement_Meta_Id,
  kFeedFilterElement_Meta_Status,
  kFeedFilterElement_Meta_Type,
  kFeedFilterElement_Meta_Episodes,
  kFeedFilterElement_Meta_DateStart,
  kFeedFilterElement_Meta_DateEnd,
  kFeedFilterElement_User_Notes,
  kFeedFilterElement_User_Status,
  kFeedFilterElement_Local_EpisodeAvailable,
  kFeedFilterElement_Episode_Title,
  kFeedFilterElement_Episode_Number,
  kFeedFilterElement_Episode_Version,
  kFeedFilterElement_Episode_Group,
  kFeedFilterElement_Episode_VideoResolution,
  kFeedFilterElement_Episode_VideoType,
  kFeedFilterElement_File_Title,
  kFeedFilterElement_File_Category,
  kFeedFilterElement_File_Description,
  kFeedFilterElement_File_Link,
  kFeedFilterElement_File_Size,
  kFeedFilterElement_Count
};

enum FeedFilterOperator {
  kFeedFilterOperator_Equals,
  kFeedFilterOperator_NotEquals,
  kFeedFilterOperator_IsGreaterThan,
  kFeedFilterOperator_IsGreaterThanOrEqualTo,
  kFeedFilterOperator_IsLessThan,
  kFeedFilterOperator_IsLessThanOrEqualTo,
  kFeedFilterOperator_BeginsWith,
  kFeedFilterOperator_EndsWith,
  kFeedFilterOperator_Contains,
  kFeedFilterOperator_NotContains,
  kFeedFilterOperator_Count
};

enum FeedFilterMatch {
  kFeedFilterMatchAll,
  kFeedFilterMatchAny
};

enum FeedFilterAction {
  kFeedFilterActionDiscard,
  kFeedFilterActionSelect,
  kFeedFilterActionPrefer
};

enum FeedFilterOption {
  kFeedFilterOptionDefault,
  kFeedFilterOptionDeactivate,
  kFeedFilterOptionHide
};

class Feed;
class FeedItem;

struct FeedFilterCondition {
  FeedFilterElement element = kFeedFilterElement_File_Title;
  FeedFilterOperator op = kFeedFilterOperator_Equals;
  std::wstring value;
};

struct FeedFilter {
  std::wstring name;
  bool enabled = true;
  FeedFilterAction action = kFeedFilterActionDiscard;
  FeedFilterMatch match = kFeedFilterMatchAll;
  FeedFilterOption option = kFeedFilterOptionDefault;
  std::vector<int> anime_ids;
  std::vector<FeedFilterCondition> conditions;
};

struct FeedFilterPreset {
  std::wstring description;
  FeedFilter filter;
  bool is_default = false;
};

bool ApplyFilter(const FeedFilter& filter, Feed& feed, FeedItem& item, bool recursive);
bool ApplyPreferenceFilter(const FeedFilter& filter, Feed& feed, FeedItem& item);

}  // namespace track
