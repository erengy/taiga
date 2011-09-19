/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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

#include "std.h"
#include "animelist.h"
#include "common.h"
#include "myanimelist.h"
#include "stats.h"
#include "string.h"

Statistics Stats;

// =============================================================================

Statistics::Statistics() : 
  anime_count(0), episode_count(0), score_mean(0.0f), score_deviation(0.0f)
{
}

// =============================================================================

void Statistics::CalculateAll() {
  // Anime count
  CalculateAnimeCount();

  // Episode count
  CalculateEpisodeCount();

  // Life spent watching
  CalculateLifeSpentWatching();

  // Mean score
  CalculateMeanScore();

  // Standard deviation of score
  CalculateScoreDeviation();
}

int Statistics::CalculateAnimeCount() {
  anime_count = AnimeList.count;
  return anime_count;
}

int Statistics::CalculateEpisodeCount() {
  episode_count = 0;
  
  for (auto it = AnimeList.items.begin() + 1; it != AnimeList.items.end(); ++it) {
    episode_count += it->GetLastWatchedEpisode();
    // TODO: implement times_rewatched when MAL adds to API
    if (it->GetRewatching() == TRUE) episode_count += it->GetTotalEpisodes();
  }

  return episode_count;
}

wstring Statistics::CalculateLifeSpentWatching() {
  int duration, seconds = 0;
  
  for (auto it = AnimeList.items.begin() + 1; it != AnimeList.items.end(); ++it) {
    // Approximate duration in minutes
    switch (it->series_type) {
      default:
      case MAL_TV:      duration = 24;  break;
      case MAL_OVA:     duration = 24;  break;
      case MAL_MOVIE:   duration = 130; break;
      case MAL_SPECIAL: duration = 12;  break;
      case MAL_ONA:     duration = 24;  break;
      case MAL_MUSIC:   duration = 5;   break;
    }
    int episodes_watched = it->GetLastWatchedEpisode();
    if (it->GetRewatching() == TRUE) episodes_watched += it->GetTotalEpisodes();
    seconds += (duration * 60) * episodes_watched;
  }
  
  if (seconds > 0) {
    life_spent_watching = ToDateString(seconds);
  } else {
    life_spent_watching = L"None";
  }

  return life_spent_watching;
}

float Statistics::CalculateMeanScore() {
  float sum_scores = 0.0f, items_scored = 0.0f;
  
  for (auto it = AnimeList.items.begin() + 1; it != AnimeList.items.end(); ++it) {
    if (it->my_score > 0) {
      sum_scores += static_cast<float>(it->my_score);
      items_scored++;
    }
  }
  
  score_mean = items_scored > 0 ? sum_scores / items_scored : 0.0f;
  
  return score_mean;
}

float Statistics::CalculateScoreDeviation() {
  float sum_squares = 0.0f, items_scored = 0.0f;
  
  for (auto it = AnimeList.items.begin() + 1; it != AnimeList.items.end(); ++it) {
    if (it->my_score > 0) {
      sum_squares += pow(static_cast<float>(it->my_score) - score_mean, 2);
      items_scored++;
    }
  }
  
  score_deviation = items_scored > 0 ? sqrt(sum_squares / items_scored) : 0.0f;

  return score_deviation;
}