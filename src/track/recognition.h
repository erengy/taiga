/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#ifndef RECOGNITION_H
#define RECOGNITION_H

#include "base/std.h"

namespace anime {
class Episode;
class Item;
}
class Token;

// =============================================================================

class RecognitionEngine {
public:
  RecognitionEngine();
  virtual ~RecognitionEngine() {};
  
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
  
  bool ExamineTitle(wstring title, 
                    anime::Episode& episode, 
                    bool examine_inside = true, 
                    bool examine_outside = true, 
                    bool examine_number = true, 
                    bool check_extras = true, 
                    bool check_extension = true);

  void ExamineToken(Token& token, anime::Episode& episode, bool compare_extras);
  
  void CleanTitle(wstring& title);
  void UpdateCleanTitles(int anime_id);

  std::multimap<int, int, std::greater<int>> GetScores();

  // Mapped as <anime_id, score>
  std::map<int, int> scores;

  std::map<int, vector<wstring>> clean_titles;

  vector<wstring> audio_keywords;
  vector<wstring> video_keywords;
  vector<wstring> extra_keywords;
  vector<wstring> extra_unsafe_keywords;
  vector<wstring> version_keywords;
  vector<wstring> valid_extensions;
  vector<wstring> episode_keywords;
  vector<wstring> episode_prefixes;

 private:
  bool CompareTitle(const wstring& anime_title, 
                    anime::Episode& episode, 
                    const anime::Item& anime_item, 
                    bool strict = true);
  bool ScoreTitle(const anime::Episode& episode, 
                  const anime::Item& anime_item);

  void AppendKeyword(wstring& str, const wstring& keyword);
  bool CompareKeys(const wstring& str, const vector<wstring>& keys);
  void EraseUnnecessary(wstring& str);
  void TransliterateSpecial(wstring& str);
  bool IsEpisodeFormat(const wstring& str, anime::Episode& episode, const wchar_t separator = ' ');
  bool IsResolution(const wstring& str);
  bool IsCountingWord(const wstring& str);
  bool IsTokenEnclosed(const Token& token);
  void ReadKeyword(unsigned int id, vector<wstring>& str);
  size_t TokenizeTitle(const wstring& str, const wstring& delimiters, vector<Token>& tokens);
  bool ValidateEpisodeNumber(anime::Episode& episode);
};

extern RecognitionEngine Meow;

#endif // RECOGNITION_H