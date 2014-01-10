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

namespace anime {
class Episode;
class Item;
}
class Token;

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

  void ExamineToken(Token& token, anime::Episode& episode, bool compare_extras);

  void CleanTitle(std::wstring& title);
  void UpdateCleanTitles(int anime_id);

  std::multimap<int, int, std::greater<int>> GetScores();

  // Mapped as <anime_id, score>
  std::map<int, int> scores;

  std::map<int, std::vector<std::wstring>> clean_titles;

  std::vector<std::wstring> audio_keywords;
  std::vector<std::wstring> video_keywords;
  std::vector<std::wstring> extra_keywords;
  std::vector<std::wstring> extra_unsafe_keywords;
  std::vector<std::wstring> version_keywords;
  std::vector<std::wstring> valid_extensions;
  std::vector<std::wstring> episode_keywords;
  std::vector<std::wstring> episode_prefixes;

private:
  bool CompareTitle(const std::wstring& anime_title,
                    anime::Episode& episode,
                    const anime::Item& anime_item,
                    bool strict = true);
  bool ScoreTitle(const anime::Episode& episode,
                  const anime::Item& anime_item);

  void AppendKeyword(std::wstring& str, const std::wstring& keyword);
  bool CompareKeys(const std::wstring& str, const std::vector<std::wstring>& keys);
  void EraseUnnecessary(std::wstring& str);
  void TransliterateSpecial(std::wstring& str);
  bool IsEpisodeFormat(const std::wstring& str, anime::Episode& episode, const wchar_t separator = ' ');
  bool IsResolution(const std::wstring& str);
  bool IsCountingWord(const std::wstring& str);
  bool IsTokenEnclosed(const Token& token);
  void ReadKeyword(std::vector<std::wstring>& output, const std::wstring& input);
  size_t TokenizeTitle(const std::wstring& str, const std::wstring& delimiters, std::vector<Token>& tokens);
  bool ValidateEpisodeNumber(anime::Episode& episode);
};

extern RecognitionEngine Meow;

#endif  // TAIGA_TRACK_RECOGNITION_H