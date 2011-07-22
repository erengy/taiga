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
  anime_count_(0), episode_count_(0), mean_score_(0.0f)
{
}

// =============================================================================

void TaigaStats::Calculate() {
  // Anime count
  anime_count_ = AnimeList.Count;

  // Episode count
  episode_count_ = 0;
  for (auto it = AnimeList.Item.begin() + 1; it != AnimeList.Item.end(); ++it) {
    episode_count_ += it->Series_Episodes;
  }

  // Life spent on watching
  int duration, days, hours, minutes, seconds = 0;
  for (auto it = AnimeList.Item.begin() + 1; it != AnimeList.Item.end(); ++it) {
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
    seconds += (duration * 60) * it->GetLastWatchedEpisode();
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

  // Mean score
  float total_score = 0.0f, items_scored = 0.0f;
  for (auto it = AnimeList.Item.begin() + 1; it != AnimeList.Item.end(); ++it) {
    if (it->My_Score > 0) {
      total_score += static_cast<float>(it->My_Score);
      items_scored++;
    }
  }
  mean_score_ = total_score / items_scored;

  #ifdef _DEBUG
  wstring msg;
  msg += L"Anime count: \t\t" + ToWSTR(anime_count_);
  msg += L"\nEpisode count: \t\t" + ToWSTR(episode_count_);
  msg += L"\nLife spent on watching: \t" + life_spent_;
  msg += L"\nMean score: \t\t" + ToWSTR(mean_score_, 2);
  MessageBox(g_hMain, msg.c_str(), L"Statistics", MB_OK | MB_ICONINFORMATION);
  #endif
}