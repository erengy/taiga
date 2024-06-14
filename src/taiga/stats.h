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

#include <array>
#include <string>

namespace taiga {

class Statistics {
public:
  void CalculateAll();
  int CalculateAnimeCount();
  int CalculateEpisodeCount();
  const std::wstring& CalculateLifePlannedToWatch();
  const std::wstring& CalculateLifeSpentWatching();
  void CalculateLocalData();
  float CalculateMeanScore();
  float CalculateScoreDeviation();
  const std::array<float, 11>& CalculateScoreDistribution();

  int anime_count = 0;
  int connections_failed = 0;
  int connections_succeeded = 0;
  int episode_count = 0;
  unsigned int image_count = 0;
  unsigned long long image_size = 0;
  std::wstring life_planned_to_watch;
  std::wstring life_spent_watching;
  float score_mean = 0.0f;
  float score_deviation = 0.0f;
  std::array<int, 11> score_count{0};
  std::array<float, 11> score_distribution{0.0f};
  int tigers_harmed = 0;
  unsigned int torrent_count = 0;
  unsigned long long torrent_size = 0;
  int uptime = 0;
};

inline taiga::Statistics stats;

}  // namespace taiga
