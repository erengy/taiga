/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#ifndef STATS_H
#define STATS_H

#include "std.h"

// =============================================================================

class Statistics {
public:
  Statistics();
  virtual ~Statistics() {}

  void CalculateAll();
  int CalculateAnimeCount();
  int CalculateEpisodeCount();
  wstring CalculateLifeSpentWatching();
  void CalculateLocalData();
  float CalculateMeanScore();
  float CalculateScoreDeviation();
  vector<float> CalculateScoreDistribution();

public:
  int anime_count;
  int episode_count;
  int image_count;
  int image_size;
  wstring life_spent_watching;
  float score_mean;
  float score_deviation;
  vector<float> score_distribution;
  int tigers_harmed;
  int uptime;
};

extern Statistics Stats;

#endif // STATS_H