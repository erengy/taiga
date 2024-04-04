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

#include "track/feed_filter.h"

namespace pugi {
class xml_node;
}

namespace track {

class FeedFilterManager {
public:
  FeedFilterManager();
  ~FeedFilterManager() {}

  void AddPresets(std::vector<FeedFilter>& filters);
  void AddPresets();
  const std::vector<FeedFilterPreset>& GetPresets() const;
  const std::vector<FeedFilter>& GetFilters() const;
  void SetFilters(const std::vector<FeedFilter>& filters);

  void Filter(Feed& feed, bool preferences);
  void FilterArchived(Feed& feed);
  void MarkNewEpisodes(Feed& feed);

  bool Import(const std::wstring& input, std::vector<FeedFilter>& filters);
  void Import(const pugi::xml_node& node_filter, std::vector<FeedFilter>& filters);
  void Import(const pugi::xml_node& node_filter);
  void Export(std::wstring& output, const std::vector<FeedFilter>& filters);
  void Export(pugi::xml_node& node_filter, const std::vector<FeedFilter>& filters);
  void Export(pugi::xml_node& node_filter);

  bool GetFansubFilter(int anime_id, std::vector<std::wstring>& groups);
  bool SetFansubFilter(int anime_id, const std::wstring& group_name, const std::wstring& video_resolution);
  bool AddDiscardFilter(int anime_id);

private:
  void InitializePresets();

  std::vector<FeedFilter> filters_;
  std::vector<FeedFilterPreset> presets_;
};

inline FeedFilterManager feed_filter_manager;

}  // namespace track
