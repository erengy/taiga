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
#include <functional>

namespace anime {
class Episode;
class Item;
}

namespace track {
namespace recognition {

class MatchOptions {
public:
  MatchOptions();

  bool check_airing_date;
  bool check_anime_type;
  bool validate_episode_number;
};

class Engine {
public:
  bool Parse(std::wstring title, anime::Episode& episode);
  int Identify(anime::Episode& episode, bool give_score, const MatchOptions& match_options);
  bool Match(anime::Episode& episode, const anime::Item& anime_item, const MatchOptions& match_options);

  void UpdateCleanTitles(int anime_id);

  typedef std::multimap<int, int, std::greater<int>> score_result_t;
  score_result_t GetScores();
  void ResetScores();
  void ScoreTitle(const anime::Episode& episode);

//private:
  int Find(anime::Episode& episode, const MatchOptions& match_options);
  bool Engine::ValidateOptions(anime::Episode& episode, const anime::Item& anime_item, const MatchOptions& match_options);
  bool ValidateEpisodeNumber(anime::Episode& episode, const anime::Item& anime_item);

  void InitializeCleanTitles();
  void CleanTitle(std::wstring& title);
  void ErasePunctuation(std::wstring& str);
  void EraseUnnecessary(std::wstring& str);
  void TransliterateSpecial(std::wstring& str);

  std::map<std::wstring, std::set<int, std::greater<int>>> clean_titles_;
  std::map<int, int> scores_;
};

}  // namespace recognition
}  // namespace track

extern track::recognition::Engine Meow;

#endif  // TAIGA_TRACK_RECOGNITION_H