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
#include <string>
#include <vector>
#include <functional>

namespace anime {
class Episode;
class Item;
}

class RecognitionEngine {
public:
  RecognitionEngine();
  ~RecognitionEngine() {};

  void Initialize();

  anime::Item* MatchDatabase(anime::Episode& episode,
                             bool in_list = true,
                             bool reverse = true,
                             bool strict = true,
                             bool check_episode = true,
                             bool check_date = true,
                             bool give_score = false);

  bool CompareEpisode(anime::Episode& episode,
                      const anime::Item& anime_item,
                      bool strict = true,
                      bool check_episode = true,
                      bool check_date = true,
                      bool give_score = false);

  bool ExamineTitle(std::wstring title,
                    anime::Episode& episode,
                    bool examine_inside = true,
                    bool examine_outside = true,
                    bool examine_number = true,
                    bool check_extras = true,
                    bool check_extension = true);

  void CleanTitle(std::wstring& title);
  void UpdateCleanTitles(int anime_id);

  std::multimap<int, int, std::greater<int>> GetScores();

  // Mapped as <anime_id, score>
  std::map<int, int> scores;

  std::map<int, std::vector<std::wstring>> clean_titles;

private:
  bool CompareTitle(const std::wstring& anime_title,
                    anime::Episode& episode,
                    const anime::Item& anime_item,
                    bool strict = true);
  bool ScoreTitle(const anime::Episode& episode,
                  const anime::Item& anime_item);

  void EraseUnnecessary(std::wstring& str);
  void TransliterateSpecial(std::wstring& str);
};

extern RecognitionEngine Meow;

#endif  // TAIGA_TRACK_RECOGNITION_H