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

#include "track/recognition.h"

#include "base/string.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "track/episode.h"
#include "ui/translate.h"

namespace track::recognition {

sorted_scores_t Engine::GetScores() const {
  return scores_;
}

int Engine::ScoreTitle(anime::Episode& episode, const std::set<int>& anime_ids,
                       const MatchOptions& match_options) {
  scores_t trigram_results;

  auto normal_title = episode.anime_title();
  Normalize(normal_title, kNormalizeForTrigrams, false);

  trigram_container_t t1;
  GetTrigrams(normal_title, t1);

  auto calculate_trigram_results = [&](int anime_id) {
    for (const auto& t2 : db_[anime_id].trigrams) {
      double result = CompareTrigrams(t1, t2);
      if (result > 0.1) {
        auto& target = trigram_results[anime_id];
        target = std::max(target, result);
      }
    }
  };

  if (!anime_ids.empty()) {
    for (const auto& id : anime_ids) {
      calculate_trigram_results(id);
    }
  } else {
    for (const auto& it : anime::db.items) {
      if (ValidateOptions(episode, it.second, match_options, false))
        calculate_trigram_results(it.first);
    }
  }

  return ScoreTitle(normal_title, episode, trigram_results);
}

static double CustomScore(const std::wstring& title, const std::wstring& str) {
  double length_min = static_cast<double>(std::min(title.size(), str.size()));
  double length_max = static_cast<double>(std::max(title.size(), str.size()));
  double length_ratio = length_min / length_max;

  double score = 0.0;

  if (StartsWith(title, str) || StartsWith(str, title)) {
    score = length_ratio;
  } else if (InStr(title, str) > -1 || InStr(str, title) > -1) {
    score = length_ratio * 0.9;
  } else {
    auto length_lcs = LongestCommonSubsequenceLength(title, str);
    auto lcs_score = length_lcs / length_max;
    score = lcs_score * 0.8;

    auto mismatch = std::mismatch(title.begin(), title.end(),
                                  str.begin(), str.end());
    auto distance = std::distance(title.begin(), mismatch.first);
    if (distance > 0) {
      auto distance_score = distance / length_min;
      score = std::max(score, distance_score * 0.7);
    }
  }

  return score;
};

static double BonusScore(const anime::Episode& episode, int id) {
  double score = 0.0;
  auto anime_item = anime::db.Find(id);

  if (anime_item) {
    auto anime_year = episode.anime_year();
    if (anime_year)
      if (anime_year == anime_item->GetDateStart().year())
        score += 0.1;

    auto anime_type = ui::TranslateType(episode.anime_type());
    if (anime_type != anime::SeriesType::Unknown)
      if (anime_type == anime_item->GetType())
        score += 0.1;
  }

  return score;
};

int Engine::ScoreTitle(const std::wstring& str, const anime::Episode& episode,
                       const scores_t& trigram_results) {
  scores_t jaro_winkler, levenshtein, custom, bonus;

  scores_.clear();

  for (const auto& trigram_result : trigram_results) {
    int id = trigram_result.first;

    // Calculate individual scores for all titles
    for (auto& title : db_[id].normal_titles) {
      jaro_winkler[id] = std::max(jaro_winkler[id], JaroWinklerDistance(title, str));
      levenshtein[id] = std::max(levenshtein[id], LevenshteinDistance(title, str));
      custom[id] = std::max(custom[id], CustomScore(title, str));
    }
    bonus[id] = BonusScore(episode, id);

    // Calculate the average score for the ID
    double score =
        (((1.0 * jaro_winkler[id]) +
          (0.5 * std::pow(custom[id], 0.66)) +
          (0.3 * std::pow(levenshtein[id], 0.8)) +
          (0.2 * std::pow(trigram_result.second, 0.8))) / 2.0) + bonus[id];
    if (score >= 0.3)
      scores_.push_back(std::make_pair(id, score));
  }

  // Sort scores in descending order, then limit the results
  std::stable_sort(scores_.begin(), scores_.end(),
      [&](const std::pair<int, double>& a,
          const std::pair<int, double>& b) {
        return a.second > b.second;
      });
  if (scores_.size() > 20)
    scores_.resize(20);

  double score_1st = scores_.size() > 0 ? scores_.at(0).second : 0.0;
  double score_2nd = scores_.size() > 1 ? scores_.at(1).second : 0.0;

  if (score_1st >= 1.0 && score_1st != score_2nd)
    return scores_.front().first;

  return anime::ID_UNKNOWN;
}

}  // namespace track::recognition
