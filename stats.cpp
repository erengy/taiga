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
#include "myanimelist.h"
#include "stats.h"
#include "string.h"

TaigaStats Stats;

// =============================================================================

TaigaStats::TaigaStats() : 
  anime_count_(0), episode_count_(0), score_mean_(0.0f), score_dev_(0.0f)
{
}

// =============================================================================

void TaigaStats::CalculateAll() {
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

int TaigaStats::CalculateAnimeCount() {
  anime_count_ = AnimeList.Count;
  return anime_count_;
}

int TaigaStats::CalculateEpisodeCount() {
  episode_count_ = 0;
  
  for (auto it = AnimeList.Items.begin() + 1; it != AnimeList.Items.end(); ++it) {
    episode_count_ += it->GetLastWatchedEpisode();
    // TODO: implement times_rewatched when MAL adds to API
    if (it->GetRewatching() == TRUE) episode_count_ += it->GetTotalEpisodes();
  }

  return episode_count_;
}

wstring TaigaStats::CalculateLifeSpentWatching() {
  int duration, days, hours, minutes, seconds = 0;
  
  for (auto it = AnimeList.Items.begin() + 1; it != AnimeList.Items.end(); ++it) {
    // Approximate duration in minutes
    switch (it->Series_Type) {
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
    #define CALC_TIME(x, y) x = seconds / (y); seconds = seconds % (y);
    CALC_TIME(days, 60 * 60 * 24);
    CALC_TIME(hours, 60 * 60);
    CALC_TIME(minutes, 60);
    #undef CALC_TIME
    life_spent_.clear();
    #define ADD_TIME(x, y) \
      if (x > 0) { \
        if (!life_spent_.empty()) life_spent_ += L" "; \
        life_spent_ += ToWSTR(static_cast<int>(x)) + y; \
        if (x > 1) life_spent_ += L"s"; \
      }
    ADD_TIME(days, L" day");
    ADD_TIME(hours, L" hour");
    ADD_TIME(minutes, L" minute");
    ADD_TIME(seconds, L" second");
    #undef ADD_TIME
  } else {
    life_spent_ = L"None";
  }

  return life_spent_;
}

float TaigaStats::CalculateMeanScore() {
  float sum_scores = 0.0f, items_scored = 0.0f;
  
  for (auto it = AnimeList.Items.begin() + 1; it != AnimeList.Items.end(); ++it) {
    if (it->My_Score > 0) {
      sum_scores += static_cast<float>(it->My_Score);
      items_scored++;
    }
  }
  
  score_mean_ = items_scored > 0 ? sum_scores / items_scored : 0.0f;
  
  return score_mean_;
}

float TaigaStats::CalculateScoreDeviation() {
  float sum_squares = 0.0f, items_scored = 0.0f;
  
  for (auto it = AnimeList.Items.begin() + 1; it != AnimeList.Items.end(); ++it) {
    if (it->My_Score > 0) {
      sum_squares += pow(static_cast<float>(it->My_Score) - score_mean_, 2);
      items_scored++;
    }
  }
  
  score_dev_ = items_scored > 0 ? sqrt(sum_squares / items_scored) : 0.0f;

  return score_dev_;
}