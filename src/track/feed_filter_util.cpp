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

#include <map>

#include "track/feed_filter_util.h"

#include "base/format.h"
#include "base/log.h"
#include "base/string.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "ui/translate.h"

namespace track::util {

std::wstring CreateNameFromConditions(const FeedFilter& filter) {
  // @TODO
  return L"New Filter";
}

std::wstring TranslateCondition(const FeedFilterCondition& condition) {
  return L"{} {} \"{}\""_format(TranslateElement(condition.element),
                                TranslateOperator(condition.op),
                                TranslateValue(condition));
}

std::wstring TranslateConditions(const FeedFilter& filter, const size_t index) {
  std::wstring str;

  const size_t max_index = (filter.match == kFeedFilterMatchAll)
                               ? filter.conditions.size()
                               : index + 1;

  for (size_t i = index; i < max_index; i++) {
    if (i > index)
      str += L" & ";
    str += TranslateCondition(filter.conditions[i]);
  }

  return str;
}

std::wstring TranslateElement(const int element) {
  switch (element) {
    case kFeedFilterElement_File_Title: return L"File name";
    case kFeedFilterElement_File_Category: return L"File category";
    case kFeedFilterElement_File_Description: return L"File description";
    case kFeedFilterElement_File_Link: return L"File link";
    case kFeedFilterElement_File_Size: return L"File size";
    case kFeedFilterElement_Meta_Id: return L"Anime ID";
    case kFeedFilterElement_Episode_Title: return L"Episode title";
    case kFeedFilterElement_Meta_DateStart: return L"Anime date started";
    case kFeedFilterElement_Meta_DateEnd: return L"Anime date ended";
    case kFeedFilterElement_Meta_Episodes: return L"Anime episode count";
    case kFeedFilterElement_Meta_Status: return L"Anime airing status";
    case kFeedFilterElement_Meta_Type: return L"Anime type";
    case kFeedFilterElement_User_Notes: return L"Anime notes";
    case kFeedFilterElement_User_Status: return L"Anime watching status";
    case kFeedFilterElement_Episode_Number: return L"Episode number";
    case kFeedFilterElement_Episode_Version: return L"Episode version";
    case kFeedFilterElement_Local_EpisodeAvailable: return L"Episode availability";
    case kFeedFilterElement_Episode_Group: return L"Episode fansub group";
    case kFeedFilterElement_Episode_VideoResolution: return L"Episode video resolution";
    case kFeedFilterElement_Episode_VideoType: return L"Episode video type";
    default: return L"?";
  }
}

std::wstring TranslateOperator(const int op) {
  switch (op) {
    case kFeedFilterOperator_Equals: return L"is";
    case kFeedFilterOperator_NotEquals: return L"is not";
    case kFeedFilterOperator_IsGreaterThan: return L"is greater than";
    case kFeedFilterOperator_IsGreaterThanOrEqualTo: return L"is greater than or equal to";
    case kFeedFilterOperator_IsLessThan: return L"is less than";
    case kFeedFilterOperator_IsLessThanOrEqualTo: return L"is less than or equal to";
    case kFeedFilterOperator_BeginsWith: return L"begins with";
    case kFeedFilterOperator_EndsWith: return L"ends with";
    case kFeedFilterOperator_Contains: return L"contains";
    case kFeedFilterOperator_NotContains: return L"does not contain";
    default: return L"?";
  }
}

std::wstring TranslateValue(const FeedFilterCondition& condition) {
  switch (condition.element) {
    case kFeedFilterElement_Meta_Id: {
      if (condition.value.empty()) {
        return L"(?)";
      } else {
        if (const auto anime_item = anime::db.Find(ToInt(condition.value))) {
          return condition.value + L" (" + anime::GetPreferredTitle(*anime_item) + L")";
        } else {
          return condition.value + L" (?)";
        }
      }
    }
    case kFeedFilterElement_User_Status:
      return ui::TranslateMyStatus(static_cast<anime::MyStatus>(ToInt(condition.value)), false);
    case kFeedFilterElement_Meta_Status:
      return ui::TranslateStatus(static_cast<anime::SeriesStatus>(ToInt(condition.value)));
    case kFeedFilterElement_Meta_Type:
      return ui::TranslateType(static_cast<anime::SeriesType>(ToInt(condition.value)));
    default:
      return !condition.value.empty() ? condition.value : L"(empty)";
  }
}

std::wstring TranslateMatching(const int match) {
  switch (match) {
    case kFeedFilterMatchAll: return L"All conditions";
    case kFeedFilterMatchAny: return L"Any condition";
    default: return L"?";
  }
}

std::wstring TranslateAction(const int action) {
  switch (action) {
    case kFeedFilterActionDiscard: return L"Discard matched items";
    case kFeedFilterActionSelect: return L"Select matched items";
    case kFeedFilterActionPrefer: return L"Prefer matched items to similar ones";
    default: return L"?";
  }
}

std::wstring TranslateOption(const int option) {
  switch (option) {
    case kFeedFilterOptionDefault: return L"Default";
    case kFeedFilterOptionDeactivate: return L"Deactivate discarded items";
    case kFeedFilterOptionHide: return L"Hide discarded items";
    default: return L"?";
  }
}

////////////////////////////////////////////////////////////////////////////////

static const std::map<int, std::wstring>& GetShortcodeMap(const Shortcode type) {
  static std::map<int, std::wstring> action_shortcodes = {
      {kFeedFilterActionDiscard, L"discard"},
      {kFeedFilterActionSelect, L"select"},
      {kFeedFilterActionPrefer, L"prefer"},
  };
  static std::map<int, std::wstring> element_shortcodes = {
      {kFeedFilterElement_Meta_Id, L"meta_id"},
      {kFeedFilterElement_Meta_Status, L"meta_status"},
      {kFeedFilterElement_Meta_Type, L"meta_type"},
      {kFeedFilterElement_Meta_Episodes, L"meta_episodes"},
      {kFeedFilterElement_Meta_DateStart, L"meta_date_start"},
      {kFeedFilterElement_Meta_DateEnd, L"meta_date_end"},
      {kFeedFilterElement_User_Notes, L"user_tags"},
      {kFeedFilterElement_User_Status, L"user_status"},
      {kFeedFilterElement_Local_EpisodeAvailable, L"local_episode_available"},
      {kFeedFilterElement_Episode_Title, L"episode_title"},
      {kFeedFilterElement_Episode_Number, L"episode_number"},
      {kFeedFilterElement_Episode_Version, L"episode_version"},
      {kFeedFilterElement_Episode_Group, L"episode_group"},
      {kFeedFilterElement_Episode_VideoResolution, L"episode_video_resolution"},
      {kFeedFilterElement_Episode_VideoType, L"episode_video_type"},
      {kFeedFilterElement_File_Title, L"file_title"},
      {kFeedFilterElement_File_Category, L"file_category"},
      {kFeedFilterElement_File_Description, L"file_description"},
      {kFeedFilterElement_File_Link, L"file_link"},
      {kFeedFilterElement_File_Size, L"file_size"},
  };
  static std::map<int, std::wstring> match_shortcodes = {
      {kFeedFilterMatchAll, L"all"},
      {kFeedFilterMatchAny, L"any"},
  };
  static std::map<int, std::wstring> operator_shortcodes = {
      {kFeedFilterOperator_Equals, L"equals"},
      {kFeedFilterOperator_NotEquals, L"notequals"},
      {kFeedFilterOperator_IsGreaterThan, L"gt"},
      {kFeedFilterOperator_IsGreaterThanOrEqualTo, L"ge"},
      {kFeedFilterOperator_IsLessThan, L"lt"},
      {kFeedFilterOperator_IsLessThanOrEqualTo, L"le"},
      {kFeedFilterOperator_BeginsWith, L"beginswith"},
      {kFeedFilterOperator_EndsWith, L"endswith"},
      {kFeedFilterOperator_Contains, L"contains"},
      {kFeedFilterOperator_NotContains, L"notcontains"},
  };
  static std::map<int, std::wstring> option_shortcodes = {
      {kFeedFilterOptionDefault, L"default"},
      {kFeedFilterOptionDeactivate, L"deactivate"},
      {kFeedFilterOptionHide, L"hide"},
  };

  switch (type) {
    case Shortcode::Action: return action_shortcodes;
    case Shortcode::Element: return element_shortcodes;
    case Shortcode::Match: return match_shortcodes;
    case Shortcode::Operator: return operator_shortcodes;
    case Shortcode::Option: return option_shortcodes;
  }

  static std::map<int, std::wstring> empty;
  return empty;  // suppress warning C4715
}

std::wstring GetShortcodeFromIndex(const Shortcode type, const int index) {
  const auto& map = GetShortcodeMap(type);
  const auto it = map.find(index);
  return it != map.end() ? it->second : std::wstring{};
}

int GetIndexFromShortcode(const Shortcode type, const std::wstring& shortcode) {
  const auto& map = GetShortcodeMap(type);

  for (const auto& [index, text] : map) {
    if (IsEqual(text, shortcode))
      return index;
  }

  LOGD(L"\"{}\" for type \"{}\" not found.", shortcode, static_cast<int>(type));
  return -1;
}

}  // namespace track::util
