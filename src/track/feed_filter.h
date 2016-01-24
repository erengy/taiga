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

#ifndef TAIGA_TRACK_FEED_FILTER_H
#define TAIGA_TRACK_FEED_FILTER_H

#include <map>
#include <string>
#include <vector>

namespace pugi {
class xml_node;
}

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

enum FeedFilterShortcodeType {
  kFeedFilterShortcodeAction,
  kFeedFilterShortcodeElement,
  kFeedFilterShortcodeMatch,
  kFeedFilterShortcodeOperator,
  kFeedFilterShortcodeOption
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

class FeedFilterManager {
public:
  FeedFilterManager();
  ~FeedFilterManager() {}

  void InitializePresets();
  void InitializeShortcodes();

  void AddPresets();
  void AddFilter(FeedFilterAction action, FeedFilterMatch match, FeedFilterOption option, bool enabled, const std::wstring& name);
  void Cleanup();
  void Filter(Feed& feed, bool preferences);
  void FilterArchived(Feed& feed);
  void MarkNewEpisodes(Feed& feed);

  bool Import(const std::wstring& input, std::vector<FeedFilter>& filters);
  void Import(const pugi::xml_node& node_filter, std::vector<FeedFilter>& filters);
  void Export(std::wstring& output, const std::vector<FeedFilter>& filters);
  void Export(pugi::xml_node& node_filter, const std::vector<FeedFilter>& filters);

  std::wstring CreateNameFromConditions(const FeedFilter& filter);
  std::wstring TranslateCondition(const FeedFilterCondition& condition);
  std::wstring TranslateConditions(const FeedFilter& filter, size_t index);
  std::wstring TranslateElement(int element);
  std::wstring TranslateOperator(int op);
  std::wstring TranslateValue(const FeedFilterCondition& condition);
  std::wstring TranslateMatching(int match);
  std::wstring TranslateAction(int action);
  std::wstring TranslateOption(int option);

  std::wstring GetShortcodeFromIndex(FeedFilterShortcodeType type, int index);
  int GetIndexFromShortcode(FeedFilterShortcodeType type, const std::wstring& shortcode);

public:
  std::vector<FeedFilter> filters;
  std::vector<FeedFilterPreset> presets;

private:
  std::map<int, std::wstring> action_shortcodes_;
  std::map<int, std::wstring> element_shortcodes_;
  std::map<int, std::wstring> match_shortcodes_;
  std::map<int, std::wstring> operator_shortcodes_;
  std::map<int, std::wstring> option_shortcodes_;
};

#endif  // TAIGA_TRACK_FEED_FILTER_H