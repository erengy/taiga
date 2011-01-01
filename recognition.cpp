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

#include "std.h"
#include "animelist.h"
#include "common.h"
#include "media.h"
#include "myanimelist.h"
#include "recognition.h"
#include "resource.h"
#include "string.h"

CRecognition Meow;

class CToken {
public:
  CToken() { Virgin = true; }
  wstring Content, Encloser, Seperator;
  bool Virgin;
};

void AddCommaString(wstring& str, const wstring add);
bool CheckEpisodeNumber(CEpisode& episode);
bool CompareKeys(const wstring& str, const vector<wstring>& keys);
bool IsEpisode(const wstring& str, CEpisode& episode);
bool IsResolution(const wstring& str);
bool IsTokenEnclosed(const CToken& token);
void RemoveUnnecessary(wstring& str);
size_t TokenizeTitle(const wstring& str, const wstring& delimiters, vector<CToken>& tokens);

// =============================================================================

inline void ReadKeyword(UINT uID, vector<wstring>& str) {
  wstring str_buff; 
  ReadStringTable(uID, str_buff); 
  Split(str_buff, L", ", str);
}

CRecognition::CRecognition() {
  ReadKeyword(IDS_KEYWORD_AUDIO, AudioKeywords);
  ReadKeyword(IDS_KEYWORD_VIDEO, VideoKeywords);
  ReadKeyword(IDS_KEYWORD_EXTRA, ExtraKeywords);
  ReadKeyword(IDS_KEYWORD_EXTRA_UNSAFE, ExtraUnsafeKeywords);
  ReadKeyword(IDS_KEYWORD_VERSION, VersionKeywords);
  ReadKeyword(IDS_KEYWORD_EXTENSION, ValidExtensions);
}

// =============================================================================

bool CRecognition::CompareTitle(wstring title, int anime_index) {
  #define TRIM_ALL L"_!?.,:;-~+'`() " // !"#$%&'()*+,-./:;<=>?[\]^_`{|}~

  // Leave if not yet aired
  if (title.empty()) return false;
  if (AnimeList.Item[anime_index].Series_Status == MAL_NOTYETAIRED) {
    if (AnimeList.Item[anime_index].Series_Start.empty() || 
      AnimeList.Item[anime_index].Series_Start == L"0000-00-00" || 
      CompareStrings(GetDate(L"yyyy'-'MM'-'dd"), AnimeList.Item[anime_index].Series_Start) == -1) {
        return false;
    }
  }

  // Remove unnecessary characters
  RemoveUnnecessary(title);
  ReplaceChars(title, TRIM_ALL, L"");
  if (title.empty()) return false;

  // Compare with main title
  vector<wstring> sTitleAnime;
  sTitleAnime.insert(sTitleAnime.begin(), AnimeList.Item[anime_index].Series_Title);
  RemoveUnnecessary(sTitleAnime.front());
  ReplaceChars(sTitleAnime.front(), TRIM_ALL, L"");
  if (IsEqual(title, sTitleAnime.front())) return true;
  
  // Compare with synonyms
  Split(AnimeList.Item[anime_index].Series_Synonyms, L"; ", sTitleAnime);
  Split(AnimeList.Item[anime_index].Synonyms, L"; ", sTitleAnime);
  for (unsigned int i = 1; i < sTitleAnime.size(); i++) {
    if (sTitleAnime[i].empty()) continue;
    RemoveUnnecessary(sTitleAnime[i]);
    ReplaceChars(sTitleAnime[i], TRIM_ALL, L"");
    if (IsEqual(title, sTitleAnime[i])) return true;
  }

  return false;
  #undef TRIM_ALL
}

// =============================================================================

bool CRecognition::ExamineTitle(wstring title, CEpisode& episode, 
                                bool examine_inside, bool examine_outside, bool examine_number,
                                bool check_extras, bool check_extension) {
  // Clear previous data
  episode.Clear();

  // Remove zero width space characters
  Replace(title, L"\u200B", L"", true);
                  
  // Trim media player name
  MediaPlayers.EditTitle(title);
  if (title.empty()) return false;

  // Retrieve file name from full path
  if (title.substr(1, 2) == L":\\") {
    episode.Folder = GetPathOnly(title);
    title = GetFileName(title);
  }
  episode.File = title;

  // Check and trim file extension
  wstring extension = GetFileExtension(title);
  if (extension.length() < title.length() && extension.length() <= 5) {
    if (IsAlphanumeric(extension) && CheckFileExtension(extension, ValidExtensions)) {
      episode.Format = ToUpper_Copy(extension);
      title.resize(title.length() - extension.length() - 1);
    } else {
      if (check_extension) return false;
    }
  }

  // Tokenize
  vector<CToken> tokens;
  TokenizeTitle(title, L"[](){}", tokens);
  if (tokens.empty()) return false;
  title.clear();

  // Examine tokens
  for (unsigned int i = 0; i < tokens.size(); i++) {
    if (IsTokenEnclosed(tokens[i])) {
      if (examine_inside) ExamineToken(tokens[i], episode, check_extras);
    } else {
      if (examine_outside) ExamineToken(tokens[i], episode, check_extras);
    }
  }

  // Tidy tokens
  for (unsigned int i = 1; i < tokens.size() - 1; i++) {
    if (tokens[i - 1].Virgin == false || tokens[i].Virgin == false || 
      IsTokenEnclosed(tokens[i - 1]) == true || tokens[i].Encloser != L"(" || 
      tokens[i - 1].Content.length() < 2) {
        continue;
    }
    tokens[i - 1].Content += L"(" + tokens[i].Content + L")";
    if (IsTokenEnclosed(tokens[i + 1]) == false) {
      tokens[i - 1].Content += tokens[i + 1].Content;
      if (tokens[i - 1].Seperator.empty()) tokens[i - 1].Seperator = tokens[i + 1].Seperator;
      tokens.erase(tokens.begin() + i + 1);
    }
    tokens.erase(tokens.begin() + i);
    i = 0;
  }
  for (unsigned int i = 0; i < tokens.size(); i++) {
    Trim(tokens[i].Content, tokens[i].Seperator.c_str());
    if (tokens[i].Content.length() < 2) {
      tokens.erase(tokens.begin() + i);
      i--;
    }
  }

  // Get group name and title
  int group_index = -1, title_index = -1;
  vector<int> group_vector, title_vector;
  for (unsigned int i = 0; i < tokens.size(); i++) {
    if (IsTokenEnclosed(tokens[i])) {
      group_vector.push_back(i);
    } else {
      title_vector.push_back(i);
    }
  }
  if (title_vector.size() > 0) {
    title_index = title_vector[0];
    title_vector.erase(title_vector.begin());
  } else {
    if (group_vector.size() > 1) {
      title_index = group_vector[1];
      group_vector.erase(group_vector.begin() + 1);
    } else if (group_vector.size() > 0) {
      title_index = group_vector[0];
      group_vector.erase(group_vector.begin());
    }
  }
  for (unsigned int i = 0; i < group_vector.size(); i++) {
    if (tokens[group_vector[i]].Virgin) {
      group_index = group_vector[i];
      group_vector.erase(group_vector.begin() + i);
      break;
    }
  }
  if (group_index == -1) {
    if (title_vector.size() > 0) {
      group_index = title_vector.back();
      title_vector.erase(title_vector.end() - 1);
    }
  }
  if (title_index > -1) {
    ReplaceChars(tokens[title_index].Content, tokens[title_index].Seperator.c_str(), L" ");
    Trim(tokens[title_index].Content, L" -");
    title = tokens[title_index].Content;
    tokens[title_index].Content.clear();
  }
  if (group_index > -1) {
    if (!IsTokenEnclosed(tokens[group_index])) {
      ReplaceChars(tokens[group_index].Content, tokens[group_index].Seperator.c_str(), L" ");
      Trim(tokens[group_index].Content, L" -");
    }
    episode.Group = tokens[group_index].Content;
    tokens[group_index].Content.clear();
  }

  // Get episode number and version
  if (examine_number && episode.Number.empty()) {
    for (int i = title.length() - 1; i >= 0; i--) {
      if (IsNumeric(title[i])) {
        episode.Number = title[i] + episode.Number;
      } else {
        if (episode.Number.empty()) continue;
        switch (title[i]) {
          case '-': case '&':
            TrimLeft(episode.Number, L"0");
            episode.Number = L"-" + episode.Number;
            break;
          case 'v': case 'V':
            episode.Version = episode.Number;
            episode.Number.clear();
            break;
          case '(': case ')':
            episode.Number.clear();
            i = 1;
            break;
          case '.': {
            int pos = InStrRev(title, L" ", i);
            i = pos > -1 ? pos : 1;
            episode.Number.clear();
            break;
          }
          default:
            wstring str_left = title.substr(0, i + 1);
            wstring str_right = title.substr(i + 1 + episode.Number.length());
            Replace(str_left, L"  ", L" ", true);
            Replace(str_right, L"  ", L" ", true);
            TrimLeft(episode.Number, L"-0");
            TrimRight(str_left);
            TrimLeft(str_right);
            EraseRight(str_left, L" ep.", true);
            EraseRight(str_left, L" ep", true);
            EraseRight(str_left, L" episode", true);
            EraseRight(str_left, L" vol", true);
            TrimRight(str_left, L" -");
            TrimLeft(str_right, L" -");
            title = str_left;
            if (str_right.length() > 2) episode.Name = str_right;
            i = 0;
            break;
        }
      }
    }
  }

  // Set the final title, hopefully name of the anime
  if (title.empty()) {
    return false;
  } else {
    episode.Title = title;
    return true;
  }
}

// =============================================================================

void CRecognition::ExamineToken(CToken& token, CEpisode& episode, bool compare_extras) {
  // Split into words
  // The most common non-alphanumeric character is the seperator
  vector<wstring> words;
  token.Seperator = GetMostCommonCharacter(token.Content);
  Split(token.Content, token.Seperator, words);
  
  // Revert if there are words that are too short
  // This prevents splitting group names like "m.3.3.w" and keywords like "H.264"
  if (IsTokenEnclosed(token)) {
    for (unsigned int i = 0; i < words.size(); i++) {
      if (words[i].length() == 1) {
        words.clear(); words.push_back(token.Content); break;
      }
    }
  }

  // Compare with keywords
  for (unsigned int i = 0; i < words.size(); i++) {
    if (words[i].empty()) continue;
    #define RemoveWordFromToken(b) { \
      Replace(token.Content, words[i], L"", false, b); token.Virgin = false; }
    
    // Checksum
    if (episode.Checksum.empty() && words[i].length() == 8 && IsHex(words[i])) {
      episode.Checksum = words[i];
      RemoveWordFromToken(false);
    // Video resolution
    } else if (episode.Resolution.empty() && IsResolution(words[i])) {
      episode.Resolution = words[i];
      RemoveWordFromToken(false);
    // Video info
    } else if (CompareKeys(words[i], Meow.VideoKeywords)) {
      AddCommaString(episode.VideoType, words[i]);
      RemoveWordFromToken(true);
    // Audio info
    } else if (CompareKeys(words[i], Meow.AudioKeywords)) {
      AddCommaString(episode.AudioType, words[i]);
      RemoveWordFromToken(true);
    // Version
    } else if (episode.Version.empty() && CompareKeys(words[i], Meow.VersionKeywords)) {
      episode.Version.push_back(words[i].at(words[i].length() - 1));
      RemoveWordFromToken(true);
    // Episode number
    } else if (episode.Number.empty() && IsEpisode(words[i], episode)) {
      for (unsigned int j = i + 1; j < words.size(); j++)
        if (!words[j].empty()) episode.Name += (j == i + 1 ? L"" : L" ") + words[j];
      token.Content.resize(InStr(token.Content, words[i], 0));
    } else if (episode.Number.empty() && (i == 0 || i == words.size() - 1) && IsNumeric(words[i])) {
      episode.Number = words[i];
      TrimLeft(episode.Number, L"0");
      if (!CheckEpisodeNumber(episode)) continue;
      RemoveWordFromToken(false);
    // Extras
    } else if (compare_extras && CompareKeys(words[i], Meow.ExtraKeywords)) {
      AddCommaString(episode.Extra, words[i]);
      RemoveWordFromToken(true);
    } else if (compare_extras && CompareKeys(words[i], Meow.ExtraUnsafeKeywords)) {
      AddCommaString(episode.Extra, words[i]);
      if (IsTokenEnclosed(token)) RemoveWordFromToken(true);
    }
  }
}

// =============================================================================

// Helper functions

void AddCommaString(wstring& str, const wstring add) {
  str += (!str.empty() ? L" " : L"") + add;
}

bool CheckEpisodeNumber(CEpisode& episode) {
  int number = ToINT(episode.Number);
  if (number > 1000) {
    if (number > 1950 && number < 2050) {
      AddCommaString(episode.Extra, L"Year: " + episode.Number);
    }
    episode.Number.clear();
    return false;
  }
  return true;
}

bool CompareKeys(const wstring& str, const vector<wstring>& keys) {
  if (!str.empty())
    for (unsigned int i = 0; i < keys.size(); i++)
      if (IsEqual(str, keys[i]))
        return true;
  return false;
}

bool IsEpisode(const wstring& str, CEpisode& episode) {  
  if (IsEqual(str.substr(0, 2), L"Ep")) {
    // Ep##
    if (IsNumeric(str.substr(2))) {
      episode.Number = str.substr(2);
      TrimLeft(episode.Number, L"0");
      return true;
    // Ep##v#
    } else {
      int pos = InStrChars(str, L"vV", 0);
      if (pos > -1 && IsNumeric(str.substr(2, pos - 2))) {
        episode.Number = str.substr(2, str.length() - pos);
        episode.Version = str.substr(pos + 1);
        TrimLeft(episode.Number, L"0");
        return true;
      }
    }
  } else if (IsEqual(str.substr(0, 1), L"E")) {
    // E##
    if (IsNumeric(str.substr(1))) {
      episode.Number = str.substr(1);
      TrimLeft(episode.Number, L"0");
      return true;
    }
  }
  return false;
}

bool IsResolution(const wstring& str) {
  // *###x###*
  if (str.length() > 6) {
    int pos = InStr(str, L"x", 0);
    if (pos > -1 && 
      IsNumeric(str.substr(0, pos)) && 
      IsNumeric(str.substr(pos + 1))) {
        return true;
    }
  // *###p
  } else if (str.length() > 3) {
    if (str.at(str.length() - 1) == 'p' && 
      IsNumeric(str.substr(0, str.length() - 1))) {
        return true;
    }
  }
  return false;
}

bool IsTokenEnclosed(const CToken& token) {
  if (token.Encloser == L"[" || 
      token.Encloser == L"(" || 
      token.Encloser == L"{")
        return true;
  return false;
}

void RemoveUnnecessary(wstring& str) {
  Replace(str, L"The ", L"", false, true);
  Replace(str, L" The", L"", false, true);
}

size_t TokenizeTitle(const wstring& str, const wstring& delimiters, vector<CToken>& tokens) {
  size_t index_begin = str.find_first_not_of(delimiters);
  while (index_begin != wstring::npos) {
    size_t index_end = str.find_first_of(delimiters, index_begin + 1);
    tokens.resize(tokens.size() + 1);
    if (index_end == wstring::npos) {
      tokens.back().Content = str.substr(index_begin);
      break;
    } else {
      tokens.back().Content = str.substr(index_begin, index_end - index_begin);
      if (index_begin > 0) tokens.back().Encloser = str.substr(index_begin - 1, 1);
      index_begin = str.find_first_not_of(delimiters, index_end + 1);
    }
  }
  return tokens.size();
}