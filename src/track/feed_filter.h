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
  kFeedFilterElement_User_Status,
  kFeedFilterElement_User_Tags,
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

class FeedFilterCondition {
public:
  FeedFilterCondition();
  ~FeedFilterCondition() {}

  FeedFilterCondition& operator=(const FeedFilterCondition& condition);

  void Reset();

public:
  FeedFilterElement element;
  FeedFilterOperator op;
  std::wstring value;
};

class FeedFilter {
public:
  FeedFilter();
  ~FeedFilter() {}

  FeedFilter& operator=(const FeedFilter& filter);

  void AddCondition(FeedFilterElement element, FeedFilterOperator op, const std::wstring& value);
  bool Filter(Feed& feed, FeedItem& item, bool recursive);
  void Reset();

public:
  bool ApplyPreferenceFilter(Feed& feed, FeedItem& item);

  std::wstring name;
  bool enabled;

  FeedFilterAction action;
  FeedFilterMatch match;
  FeedFilterOption option;

  std::vector<int> anime_ids;
  std::vector<FeedFilterCondition> conditions;
};

class FeedFilterPreset {
public:
  FeedFilterPreset();
  ~FeedFilterPreset() {}

  std::wstring description;
  FeedFilter filter;
  bool is_default;
};

}  // namespace track
