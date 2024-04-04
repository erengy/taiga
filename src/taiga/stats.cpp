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

#include <algorithm>
#include <cmath>
#include <vector>

#include "taiga/stats.h"

#include "base/file.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "taiga/path.h"

namespace taiga {

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

  for (const auto& [id, item] : anime::db.items) {
    if (item.IsInList())
      ++anime_count;
  }

  return anime_count;
}

int Statistics::CalculateEpisodeCount() {
  episode_count = 0;

  for (const auto& [id, item] : anime::db.items) {
    if (!item.IsInList())
      continue;

    episode_count += item.GetMyLastWatchedEpisode();
    episode_count += item.GetMyRewatchedTimes() *
                     item.GetEpisodeCount();
  }

  return episode_count;
}

const std::wstring& Statistics::CalculateLifePlannedToWatch() {
  int seconds = 0;

  for (const auto& [id, item] : anime::db.items) {
    switch (item.GetMyStatus()) {
      case anime::MyStatus::NotInList:
      case anime::MyStatus::Completed:
      case anime::MyStatus::Dropped:
        continue;
    }

    const int episodes =
        EstimateEpisodeCount(item) - item.GetMyLastWatchedEpisode();

    seconds += (EstimateDuration(item) * 60) * episodes;
  }

  life_planned_to_watch = seconds > 0 ? ToDateString(seconds) : L"None";
  return life_planned_to_watch;
}

const std::wstring& Statistics::CalculateLifeSpentWatching() {
  int seconds = 0;

  for (const auto& [id, item] : anime::db.items) {
    if (!item.IsInList())
      continue;

    int episodes_watched = item.GetMyLastWatchedEpisode();
    episodes_watched += item.GetMyRewatchedTimes() * item.GetEpisodeCount();

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

  const auto path = taiga::GetPath(taiga::Path::Feed);
  torrent_count = PopulateFiles(file_list, path, L"torrent", true);
  torrent_size = GetFolderSize(path, true);
}

float Statistics::CalculateMeanScore() {
  float items_scored = 0.0f;
  float sum_scores = 0.0f;

  for (const auto& [id, item] : anime::db.items) {
    if (!item.IsInList())
      continue;

    if (item.GetMyScore() > 0) {
      sum_scores += static_cast<float>(item.GetMyScore());
      items_scored++;
    }
  }

  score_mean = items_scored > 0 ? (sum_scores / items_scored) : 0.0f;

  return score_mean;
}

float Statistics::CalculateScoreDeviation() {
  float items_scored = 0.0f;
  float sum_squares = 0.0f;

  for (const auto& [id, item] : anime::db.items) {
    if (!item.IsInList())
      continue;

    if (item.GetMyScore() > 0) {
      float score = static_cast<float>(item.GetMyScore());
      sum_squares += std::powf(score - score_mean, 2.0f);
      items_scored++;
    }
  }

  score_deviation =
      items_scored > 0 ? std::sqrt(sum_squares / items_scored) : 0.0f;

  return score_deviation;
}

const std::array<float, 11>& Statistics::CalculateScoreDistribution() {
  score_count.fill(0);
  score_distribution.fill(0.0f);

  float extreme_value = 1.0f;

  for (const auto& [id, item] : anime::db.items) {
    const int score = item.GetMyScore();
    if (score > 0) {
      const auto score_index = static_cast<size_t>(std::floor(score / 10.0));
      ++score_count[score_index];
      ++score_distribution[score_index];
      extreme_value = std::max(score_distribution[score_index], extreme_value);
    }
  }

  for (auto& value : score_distribution) {
    value = value / extreme_value;
  }

  return score_distribution;
}

}  // namespace taiga
