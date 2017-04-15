/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#include <algorithm>

#include "base/file.h"
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
  CalculateLifePlannedToWatch();
  CalculateLifeSpentWatching();
  CalculateLocalData();
  CalculateMeanScore();
  CalculateScoreDeviation();
  CalculateScoreDistribution();
}

int Statistics::CalculateAnimeCount() {
  anime_count = 0;

  for (const auto& pair : AnimeDatabase.items)
    if (pair.second.IsInList())
      anime_count++;

  return anime_count;
}

int Statistics::CalculateEpisodeCount() {
  episode_count = 0;

  for (const auto& pair : AnimeDatabase.items) {
    if (!pair.second.IsInList())
      continue;

    episode_count += pair.second.GetMyLastWatchedEpisode();
    episode_count += anime::GetMyRewatchedTimes(pair.second) *
                     pair.second.GetEpisodeCount();
  }

  return episode_count;
}

const std::wstring& Statistics::CalculateLifePlannedToWatch() {
  int seconds = 0;

  for (const auto& pair : AnimeDatabase.items) {
    const auto& item = pair.second;

    switch (item.GetMyStatus()) {
      case anime::kNotInList:
      case anime::kCompleted:
      case anime::kDropped:
        continue;
    }

    int episodes = EstimateEpisodeCount(item) - item.GetMyLastWatchedEpisode();

    seconds += (EstimateDuration(item) * 60) * episodes;
  }

  life_planned_to_watch = seconds > 0 ? ToDateString(seconds) : L"None";
  return life_planned_to_watch;
}

const std::wstring& Statistics::CalculateLifeSpentWatching() {
  int seconds = 0;

  for (const auto& pair : AnimeDatabase.items) {
    const auto& item = pair.second;

    if (!item.IsInList())
      continue;

    int episodes_watched = item.GetMyLastWatchedEpisode();
    episodes_watched += anime::GetMyRewatchedTimes(item) *
                        item.GetEpisodeCount();

    seconds += (EstimateDuration(item) * 60) * episodes_watched;
  }

  life_spent_watching = seconds > 0 ? ToDateString(seconds) : L"None";
  return life_spent_watching;
}

void Statistics::CalculateLocalData() {
  std::vector<std::wstring> file_list;

  image_count = PopulateFiles(file_list, anime::GetImagePath());
  image_size = GetFolderSize(anime::GetImagePath(), false);

  file_list.clear();
  std::wstring path = taiga::GetPath(taiga::Path::Feed);

  torrent_count = PopulateFiles(file_list, path, L"torrent", true);
  torrent_size = GetFolderSize(path, true);
}

float Statistics::CalculateMeanScore() {
  float items_scored = 0.0f;
  float sum_scores = 0.0f;

  for (const auto& pair : AnimeDatabase.items) {
    if (!pair.second.IsInList())
      continue;

    if (pair.second.GetMyScore() > 0) {
      sum_scores += static_cast<float>(pair.second.GetMyScore());
      items_scored++;
    }
  }

  score_mean = items_scored > 0 ? (sum_scores / items_scored) : 0.0f;

  return score_mean;
}

float Statistics::CalculateScoreDeviation() {
  float items_scored = 0.0f;
  float sum_squares = 0.0f;

  for (const auto& pair : AnimeDatabase.items) {
    if (!pair.second.IsInList())
      continue;

    if (pair.second.GetMyScore() > 0) {
      float score = static_cast<float>(pair.second.GetMyScore());
      sum_squares += pow(score - score_mean, 2);
      items_scored++;
    }
  }

  score_deviation = items_scored > 0 ? sqrt(sum_squares / items_scored) : 0.0f;

  return score_deviation;
}

const std::vector<float>& Statistics::CalculateScoreDistribution() {
  for (auto& value : score_count)
    value = 0;
  for (auto& value : score_distribution)
    value = 0.0f;

  float extreme_value = 1.0f;

  for (const auto& pair : AnimeDatabase.items) {
    int score = pair.second.GetMyScore();
    if (score > 0) {
      score_count[score]++;
      score_distribution[score]++;
      extreme_value = std::max(score_distribution[score], extreme_value);
    }
  }

  for (auto& value : score_distribution)
    value = value / extreme_value;

  return score_distribution;
}

}  // namespace taiga