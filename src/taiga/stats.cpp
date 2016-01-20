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

#include "base/file.h"
#include "base/foreach.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "taiga/path.h"
#include "taiga/stats.h"

taiga::Statistics Stats;

namespace taiga {

Statistics::Statistics()
    : anime_count(0),
      connections_failed(0),
      connections_succeeded(0),
      episode_count(0),
      image_count(0),
      image_size(0),
      score_mean(0.0f),
      score_deviation(0.0f),
      score_count(11, 0),
      score_distribution(11, 0.0f),
      tigers_harmed(0),
      torrent_count(0),
      torrent_size(0),
      uptime(0) {
}

void Statistics::CalculateAll() {
  CalculateAnimeCount();
  CalculateEpisodeCount();
  CalculateLifeSpentWatching();
  CalculateLocalData();
  CalculateMeanScore();
  CalculateScoreDeviation();
  CalculateScoreDistribution();
}

int Statistics::CalculateAnimeCount() {
  anime_count = 0;

  foreach_(it, AnimeDatabase.items)
    if (it->second.IsInList())
      anime_count++;

  return anime_count;
}

int Statistics::CalculateEpisodeCount() {
  episode_count = 0;

  foreach_(it, AnimeDatabase.items) {
    if (!it->second.IsInList())
      continue;

    episode_count += it->second.GetMyLastWatchedEpisode();
    episode_count += anime::GetMyRewatchedTimes(it->second) *
                     it->second.GetEpisodeCount();
  }

  return episode_count;
}

const std::wstring& Statistics::CalculateLifeSpentWatching() {
  int duration = 0;
  int seconds = 0;

  foreach_(it, AnimeDatabase.items) {
    if (!it->second.IsInList())
      continue;

    duration = it->second.GetEpisodeLength();
    if (duration <= 0) {
      // Approximate duration in minutes
      switch (it->second.GetType()) {
        default:
        case anime::kTv:      duration = 24; break;
        case anime::kOva:     duration = 24; break;
        case anime::kMovie:   duration = 90; break;
        case anime::kSpecial: duration = 12; break;
        case anime::kOna:     duration = 24; break;
        case anime::kMusic:   duration =  5; break;
      }
    }

    int episodes_watched = it->second.GetMyLastWatchedEpisode();
    episodes_watched += anime::GetMyRewatchedTimes(it->second) *
                        it->second.GetEpisodeCount();

    seconds += (duration * 60) * episodes_watched;
  }

  if (seconds > 0) {
    life_spent_watching = ToDateString(seconds);
  } else {
    life_spent_watching = L"None";
  }

  return life_spent_watching;
}

void Statistics::CalculateLocalData() {
  std::vector<std::wstring> file_list;

  image_count = PopulateFiles(file_list, anime::GetImagePath());
  image_size = GetFolderSize(anime::GetImagePath(), false);

  file_list.clear();
  std::wstring path = taiga::GetPath(taiga::kPathFeed);

  torrent_count = PopulateFiles(file_list, path, L"torrent", true);
  torrent_size = GetFolderSize(path, true);
}

float Statistics::CalculateMeanScore() {
  float items_scored = 0.0f;
  float sum_scores = 0.0f;

  foreach_(it, AnimeDatabase.items) {
    if (!it->second.IsInList())
      continue;

    if (it->second.GetMyScore() > 0) {
      sum_scores += static_cast<float>(it->second.GetMyScore());
      items_scored++;
    }
  }

  score_mean = items_scored > 0 ? (sum_scores / items_scored) : 0.0f;

  return score_mean;
}

float Statistics::CalculateScoreDeviation() {
  float items_scored = 0.0f;
  float sum_squares = 0.0f;

  foreach_(it, AnimeDatabase.items) {
    if (!it->second.IsInList())
      continue;

    if (it->second.GetMyScore() > 0) {
      float score = static_cast<float>(it->second.GetMyScore());
      sum_squares += pow(score - score_mean, 2);
      items_scored++;
    }
  }

  score_deviation = items_scored > 0 ? sqrt(sum_squares / items_scored) : 0.0f;

  return score_deviation;
}

const std::vector<float>& Statistics::CalculateScoreDistribution() {
  foreach_(item, score_count)
    *item = 0;
  foreach_(item, score_distribution)
    *item = 0.0f;

  float extreme_value = 1.0f;

  foreach_(it, AnimeDatabase.items) {
    int score = it->second.GetMyScore();
    if (score > 0) {
      score_count[score]++;
      score_distribution[score]++;
      extreme_value = max(score_distribution[score], extreme_value);
    }
  }

  foreach_(it, score_distribution)
    *it = *it / extreme_value;

  return score_distribution;
}

}  // namespace taiga