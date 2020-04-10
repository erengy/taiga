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

#include <map>
#include <string>
#include <vector>

#include "track/feed_filter.h"

namespace pugi {
class xml_node;
}

namespace track {

class FeedFilterManager {
public:
  FeedFilterManager();
  ~FeedFilterManager() {}

  void InitializePresets();
  void InitializeShortcodes();

  void AddPresets(std::vector<FeedFilter>& filters);
  void AddPresets();
  std::vector<FeedFilterPreset> GetPresets() const;
  void AddFilter(FeedFilterAction action, FeedFilterMatch match, FeedFilterOption option, bool enabled, const std::wstring& name);
  std::vector<FeedFilter> GetFilters() const;
  void SetFilters(const std::vector<FeedFilter>& filters);
  void Cleanup();
  void Filter(Feed& feed, bool preferences);
  void FilterArchived(Feed& feed);
  void MarkNewEpisodes(Feed& feed);

  bool Import(const std::wstring& input, std::vector<FeedFilter>& filters);
  void Import(const pugi::xml_node& node_filter, std::vector<FeedFilter>& filters);
  void Import(const pugi::xml_node& node_filter);
  void Export(std::wstring& output, const std::vector<FeedFilter>& filters);
  void Export(pugi::xml_node& node_filter, const std::vector<FeedFilter>& filters);
  void Export(pugi::xml_node& node_filter);

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

  bool GetFansubFilter(int anime_id, std::vector<std::wstring>& groups);
  bool SetFansubFilter(int anime_id, const std::wstring& group_name, const std::wstring& video_resolution);
  bool AddDiscardFilter(int anime_id);

private:
  std::vector<FeedFilter> filters_;
  std::vector<FeedFilterPreset> presets_;

  std::map<int, std::wstring> action_shortcodes_;
  std::map<int, std::wstring> element_shortcodes_;
  std::map<int, std::wstring> match_shortcodes_;
  std::map<int, std::wstring> operator_shortcodes_;
  std::map<int, std::wstring> option_shortcodes_;
};

inline FeedFilterManager feed_filter_manager;

}  // namespace track
