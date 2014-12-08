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

#ifndef TAIGA_TRACK_RECOGNITION_H
#define TAIGA_TRACK_RECOGNITION_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/string.h"

namespace anime {
class Episode;
class Item;
}

namespace track {
namespace recognition {

typedef std::map<int, double> scores_t;
typedef std::vector<std::pair<int, double>> sorted_scores_t;
typedef std::map<std::wstring, std::set<int>> title_container_t;

class MatchOptions {
public:
  MatchOptions();

  bool allow_sequels;
  bool check_airing_date;
  bool check_anime_type;
  bool validate_episode_number;
};

class Titles {
public:
  title_container_t alternative;
  title_container_t main;
  title_container_t user;
};

class Engine {
public:
  bool Parse(std::wstring title, anime::Episode& episode) const;
  int Identify(anime::Episode& episode, bool give_score, const MatchOptions& match_options);

  void UpdateTitles(const anime::Item& anime_item);

  sorted_scores_t GetScores() const;

private:
  bool ValidateOptions(anime::Episode& episode, int anime_id, const MatchOptions& match_options) const;
  int ValidateEpisodeNumber(anime::Episode& episode, const anime::Item& anime_item) const;

  void InitializeTitles();
  int LookUpTitle(const std::wstring& title, const std::wstring& normal_title, std::set<int>& anime_ids) const;

  int ScoreTitle(const anime::Episode& episode, const std::set<int>& anime_ids);
  int ScoreTitle(const std::wstring& str, const anime::Episode& episode, const scores_t& trigram_results);

  void Normalize(std::wstring& title) const;
  void NormalizeUnicode(std::wstring& str) const;
  void ErasePunctuation(std::wstring& str) const;
  void EraseUnnecessary(std::wstring& str) const;
  void ConvertOrdinalNumbers(std::wstring& str) const;
  void ConvertRomanNumbers(std::wstring& str) const;
  void ConvertSeasonNumbers(std::wstring& str) const;
  void Transliterate(std::wstring& str) const;

  Titles titles_;
  Titles normal_titles_;
  sorted_scores_t scores_;
  std::map<int, std::vector<trigram_container_t>> trigrams_;
};

}  // namespace recognition
}  // namespace track

extern track::recognition::Engine Meow;

#endif  // TAIGA_TRACK_RECOGNITION_H