/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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

class CAnime;
class CEpisode;
class CToken;

class CRecognition {
public:
  CRecognition();
  virtual ~CRecognition() {};
  
  void Initialize();
  bool CompareEpisode(CEpisode& episode, const CAnime& anime);
  bool ExamineTitle(wstring title, CEpisode& episode, 
    bool examine_inside, bool examine_outside, bool examine_number,
    bool check_extras, bool check_extension);
  void ExamineToken(CToken& token, CEpisode& episode, bool compare_extras);
  
  // Keywords
  vector<wstring> AudioKeywords, VideoKeywords, 
    ExtraKeywords, ExtraUnsafeKeywords, 
    VersionKeywords, ValidExtensions;

private:
  bool CompareTitle(const wstring& title, wstring& anime_title, 
    CEpisode& episode, const CAnime& anime);
  bool CompareSynonyms(const wstring& title, wstring& anime_title, const wstring& synonyms, 
    CEpisode& episode, const CAnime& anime);

  // Helper functions
  void AddCommaString(wstring& str, const wstring add);
  bool CheckEpisodeNumber(CEpisode& episode);
  bool CompareKeys(const wstring& str, const vector<wstring>& keys);
  void EraseUnnecessary(wstring& str);
  bool IsEpisode(const wstring& str, CEpisode& episode);
  bool IsResolution(const wstring& str);
  bool IsTokenEnclosed(const CToken& token);
  void ReadKeyword(unsigned int uID, vector<wstring>& str);
  size_t TokenizeTitle(const wstring& str, const wstring& delimiters, vector<CToken>& tokens);
};

extern CRecognition Meow;

#endif // RECOGNITION_H