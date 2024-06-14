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

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/string.h"

namespace anime {
class Episode;
class Item;
}

namespace track::recognition {

using scores_t = std::map<int, double>;
using sorted_scores_t = std::vector<std::pair<int, double>>;

struct ParseOptions {
  bool parse_path = false;
  bool streaming_media = false;
};

struct MatchOptions {
  bool allow_sequels = false;
  bool check_airing_date = false;
  bool check_anime_type = false;
  bool check_episode_number = false;
  bool streaming_media = false;
};

class Engine {
public:
  bool Parse(std::wstring filename, const ParseOptions& parse_options, anime::Episode& episode) const;
  int Identify(anime::Episode& episode, bool give_score, const MatchOptions& match_options);
  bool Search(const std::wstring& title, std::vector<int>& anime_ids);

  void InitializeTitles();
  void UpdateTitles(const anime::Item& anime_item, bool erase_ids = false);

  sorted_scores_t GetScores() const;

  bool IsBatchRelease(const anime::Episode& episode) const;
  bool IsValidAnimeType(const anime::Episode& episode) const;
  bool IsValidAnimeType(const std::wstring& path, const ParseOptions& parse_options) const;
  bool IsValidAnimeType(const std::wstring& path) const;
  bool IsValidFileExtension(const anime::Episode& episode) const;
  bool IsValidFileExtension(const std::wstring& extension) const;
  bool IsAudioFileExtension(const std::wstring& extension) const;

  bool ReadRelations();
  bool ReadRelations(const std::string& document);
  bool SearchEpisodeRedirection(int id, const std::pair<int, int>& range, int& destination_id, std::pair<int, int>& destination_range) const;

private:
  enum NormalizationType {
    kNormalizeMinimal,
    kNormalizeForTrigrams,
    kNormalizeForLookup,
    kNormalizeFull,
  };

  bool ValidateOptions(anime::Episode& episode, int anime_id, const MatchOptions& match_options, bool redirect) const;
  bool ValidateOptions(anime::Episode& episode, const anime::Item& anime_item, const MatchOptions& match_options, bool redirect) const;
  bool ValidateEpisodeNumber(anime::Episode& episode, const anime::Item& anime_item, const MatchOptions& match_options, bool redirect) const;

  int LookUpTitle(std::wstring title, std::set<int>& anime_ids) const;
  bool GetTitleFromPath(anime::Episode& episode);
  void ExtendAnimeTitle(anime::Episode& episode) const;

  int ScoreTitle(anime::Episode& episode, const std::set<int>& anime_ids, const MatchOptions& match_options);
  int ScoreTitle(const std::wstring& str, const anime::Episode& episode, const scores_t& trigram_results);

  void Normalize(std::wstring& title, int type, bool normalized_before) const;
  void NormalizeUnicode(std::wstring& str) const;
  void ErasePunctuation(std::wstring& str, int type, bool modified_tail) const;
  void EraseUnnecessary(std::wstring& str) const;
  void ConvertOrdinalNumbers(std::wstring& str) const;
  void ConvertRomanNumbers(std::wstring& str) const;
  void ConvertSeasonNumbers(std::wstring& str) const;
  void Transliterate(std::wstring& str) const;

  struct Titles {
    using container_t = std::map<std::wstring, std::set<int>>;
    container_t alternative;
    container_t main;
    container_t user;
  } normal_titles_, titles_;

  struct ScoreStore {
    std::vector<std::wstring> normal_titles;
    std::vector<trigram_container_t> trigrams;
  };
  std::map<int, ScoreStore> db_;
  sorted_scores_t scores_;
};

}  // namespace track::recognition

inline track::recognition::Engine Meow;
