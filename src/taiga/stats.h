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

#ifndef TAIGA_TAIGA_STATS_H
#define TAIGA_TAIGA_STATS_H

#include <string>
#include <vector>

namespace taiga {

class Statistics {
public:
  Statistics();
  ~Statistics() {}

  void CalculateAll();
  int CalculateAnimeCount();
  int CalculateEpisodeCount();
  const std::wstring& CalculateLifePlannedToWatch();
  const std::wstring& CalculateLifeSpentWatching();
  void CalculateLocalData();
  float CalculateMeanScore();
  float CalculateScoreDeviation();
  const std::vector<float>& CalculateScoreDistribution();

public:
  int anime_count;
  int connections_failed;
  int connections_succeeded;
  int episode_count;
  unsigned int image_count;
  unsigned long long image_size;
  std::wstring life_planned_to_watch;
  std::wstring life_spent_watching;
  float score_mean;
  float score_deviation;
  std::vector<int> score_count;
  std::vector<float> score_distribution;
  int tigers_harmed;
  unsigned int torrent_count;
  unsigned long long torrent_size;
  int uptime;
};

}  // namespace taiga

extern taiga::Statistics Stats;

#endif  // TAIGA_TAIGA_STATS_H