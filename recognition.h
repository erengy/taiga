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

#include "std.h"

// =============================================================================

class Anime;
class Episode;
class Token;

class RecognitionEngine {
public:
  RecognitionEngine();
  virtual ~RecognitionEngine() {};
  
  void Initialize();
  bool CompareEpisode(Episode& episode, const Anime& anime, 
    bool strict = true, bool check_episode = true, bool check_date = true);
  bool ExamineTitle(wstring title, Episode& episode, 
    bool examine_inside = true, bool examine_outside = true, bool examine_number = true,
    bool check_extras = true, bool check_extension = true);
  void ExamineToken(Token& token, Episode& episode, bool compare_extras);

private:
  bool CompareTitle(const wstring& title, wstring& anime_title, 
    Episode& episode, const Anime& anime, bool strict = true);
  bool CompareSynonyms(const wstring& title, wstring& anime_title, const wstring& synonyms, 
    Episode& episode, const Anime& anime, bool strict = true);

  void AppendKeyword(wstring& str, const wstring& keyword);
  bool CompareKeys(const wstring& str, const vector<wstring>& keys);
  void CleanTitle(wstring& title);
  void EraseUnnecessary(wstring& str);
  void TransliterateSpecial(wstring& str);
  bool IsEpisodeFormat(const wstring& str, Episode& episode, const wchar_t separator = ' ');
  bool IsResolution(const wstring& str);
  bool IsCountingWord(const wstring& str);
  bool IsTokenEnclosed(const Token& token);
  void ReadKeyword(unsigned int id, vector<wstring>& str);
  size_t TokenizeTitle(const wstring& str, const wstring& delimiters, vector<Token>& tokens);
  bool ValidateEpisodeNumber(Episode& episode);

public:
  vector<wstring> audio_keywords, video_keywords, 
    extra_keywords, extra_unsafe_keywords, 
    version_keywords, valid_extensions,
    episode_keywords, episode_prefixes;
};

extern RecognitionEngine Meow;

#endif // RECOGNITION_H