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
  CToken() : Encloser('\0'), Seperator('\0'), Virgin(true) {}
  wchar_t Encloser, Seperator;
  wstring Content;
  bool Virgin;
};

// =============================================================================

CRecognition::CRecognition() {
  ReadKeyword(IDS_KEYWORD_AUDIO, AudioKeywords);
  ReadKeyword(IDS_KEYWORD_VIDEO, VideoKeywords);
  ReadKeyword(IDS_KEYWORD_EXTRA, ExtraKeywords);
  ReadKeyword(IDS_KEYWORD_EXTRA_UNSAFE, ExtraUnsafeKeywords);
  ReadKeyword(IDS_KEYWORD_VERSION, VersionKeywords);
  ReadKeyword(IDS_KEYWORD_EXTENSION, ValidExtensions);
}

// =============================================================================

bool CRecognition::CompareEpisode(CEpisode& episode, const CAnime& anime, 
                                  bool strict, bool check_episode, bool check_date) {
  // Leave if title is empty
  if (episode.Title.empty()) return false;
  // Leave if episode number is out of range
  if (check_episode && anime.Series_Episodes > 1) {
    int number = GetLastEpisode(episode.Number);
    if (number == 0 || number > anime.Series_Episodes) return false;
  }
  // Leave if not yet aired
  if (check_date && anime.Series_Status == MAL_NOTYETAIRED) {
    if (anime.Series_Start.empty() || anime.Series_Start == L"0000-00-00" || 
      CompareStrings(GetDate(L"yyyy'-'MM'-'dd"), anime.Series_Start) < 0) {
        return false;
    }
  }

  // Remove unnecessary characters
  wstring title = episode.Title;
  EraseUnnecessary(title);
  ErasePunctuation(title, true);
  if (title.empty()) return false;

  // Compare with main title
  wstring anime_title = anime.Series_Title;
  if (CompareTitle(title, anime_title, episode, anime, strict)) return true;
  // Compare with synonyms
  if (CompareSynonyms(title, anime_title, anime.Series_Synonyms, episode, anime, strict)) return true;
  if (CompareSynonyms(title, anime_title, anime.Synonyms, episode, anime, strict)) return true;

  // Failed
  return false;
}

bool CRecognition::CompareTitle(const wstring& title, wstring& anime_title, 
                                CEpisode& episode, const CAnime& anime, bool strict) {
  // Remove unnecessary characters
  if (anime_title.empty()) return false;
  EraseUnnecessary(anime_title);
  ErasePunctuation(anime_title, true);
  if (anime_title.empty()) return false;
  
  // Compare with title & episode number
  if (strict && anime.Series_Episodes == 1 && !episode.Number.empty()) {
    if (IsEqual(title + episode.Number, anime_title)) {
      episode.Title += episode.Number;
      episode.Number.clear();
      return true;
    }
  }
  // Compare with title
  if (strict) {
    if (IsEqual(title, anime_title)) return true;
  } else {
    if (InStr(title, anime_title, 0, true) > -1) return true;
  }

  // Failed
  return false;
}

bool CRecognition::CompareSynonyms(const wstring& title, wstring& anime_title, const wstring& synonyms, 
                                   CEpisode& episode, const CAnime& anime, bool strict) {
  size_t index_begin = 0, index_end;
  do {
    index_end = synonyms.find(L"; ", index_begin);
    if (index_end == wstring::npos) index_end = synonyms.length();
    anime_title = synonyms.substr(index_begin, index_end - index_begin);
    if (CompareTitle(title, anime_title, episode, anime, strict)) return true;
    index_begin = index_end + 2;
  } while (index_begin <= synonyms.length());
  return false;
}

// =============================================================================

bool CRecognition::ExamineTitle(wstring title, CEpisode& episode, 
                                bool examine_inside, bool examine_outside, bool examine_number,
                                bool check_extras, bool check_extension) {
  // Clear previous data
  episode.Clear();

  // Remove zero width space character
  Erase(title, L"\u200B");
                  
  // Trim media player name
  MediaPlayers.EditTitle(title);
  if (title.empty()) return false;

  // Retrieve file name from full path
  if (title.length() > 2 && title.at(1) == ':' && title.at(2) == '\\') {
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
  tokens.reserve(4);
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

  // Tidy up tokens
  for (unsigned int i = 1; i < tokens.size() - 1; i++) {
    if (tokens[i - 1].Virgin == false || tokens[i].Virgin == false || 
      IsTokenEnclosed(tokens[i - 1]) == true || tokens[i].Encloser != '(' || 
      tokens[i - 1].Content.length() < 2) {
        continue;
    }
    tokens[i - 1].Content += L"(" + tokens[i].Content + L")";
    if (IsTokenEnclosed(tokens[i + 1]) == false) {
      tokens[i - 1].Content += tokens[i + 1].Content;
      if (tokens[i - 1].Seperator == '\0')
        tokens[i - 1].Seperator = tokens[i + 1].Seperator;
      tokens.erase(tokens.begin() + i + 1);
    }
    tokens.erase(tokens.begin() + i);
    i = 0;
  }
  for (unsigned int i = 0; i < tokens.size(); i++) {
    wchar_t trim_char[] = {tokens[i].Seperator};
    Trim(tokens[i].Content, trim_char);
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
    ReplaceChar(tokens[title_index].Content, tokens[title_index].Seperator, ' ');
    Trim(tokens[title_index].Content, L" -");
    title = tokens[title_index].Content;
    tokens[title_index].Content.clear();
  }
  if (group_index > -1) {
    if (!IsTokenEnclosed(tokens[group_index])) {
      ReplaceChar(tokens[group_index].Content, tokens[group_index].Seperator, ' ');
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
            episode.Number = L"-" + episode.Number;
            break;
          case 'v': case 'V':
            episode.Version = episode.Number;
            episode.Number.clear();
            break;
          case '(': case ')':
            episode.Number.clear();
            i = 0;
            break;
          case '.':
            i = InStrRev(title, L" ", i);
            episode.Number.clear();
            break;
          case ' ': {
            // Break title into two parts
            wstring str_left = title.substr(0, i + 1);
            wstring str_right = title.substr(i + 1 + episode.Number.length());
            // Tidy up strings
            Replace(str_left, L"  ", L" ", true);
            Replace(str_right, L"  ", L" ", true);
            TrimLeft(episode.Number, L"-");
            TrimRight(str_left);
            TrimLeft(str_right);
            const wchar_t* erase_right[4] = {
              L" ep.", L" ep", L" episode", L" vol"
            };
            for (int j = 0; j < 4; j++) {
              EraseRight(str_left, erase_right[j], true);
            }
            TrimRight(str_left, L" -");
            TrimLeft(str_right, L" -");
            // Return title and name
            title = str_left;
            if (str_right.length() > 2) {
              episode.Name = str_right;
            }
            i = 0;
            break;
          }
          default:
            episode.Number.clear();
            break;
        }
      }
    }
  }

  // Examine remaining tokens once more
  for (unsigned int i = 0; i < tokens.size(); i++) {
    if (!tokens[i].Content.empty()) {
      ExamineToken(tokens[i], episode, true);
    }
  }

  // Set the final title, hopefully name of the anime
  episode.Title = title;
  return !title.empty();
}

// =============================================================================

void CRecognition::ExamineToken(CToken& token, CEpisode& episode, bool compare_extras) {
  // Split into words
  // The most common non-alphanumeric character is the seperator
  vector<wstring> words;
  token.Seperator = GetMostCommonCharacter(token.Content);
  Split(token.Content, wstring(1, token.Seperator), words);
  
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
      Erase(token.Content, words[i], b); token.Virgin = false; }
    
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
      AppendKeyword(episode.VideoType, words[i]);
      RemoveWordFromToken(true);
    // Audio info
    } else if (CompareKeys(words[i], Meow.AudioKeywords)) {
      AppendKeyword(episode.AudioType, words[i]);
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
      if (!ValidateEpisodeNumber(episode)) continue;
      RemoveWordFromToken(false);
    // Extras
    } else if (compare_extras && CompareKeys(words[i], Meow.ExtraKeywords)) {
      AppendKeyword(episode.Extra, words[i]);
      RemoveWordFromToken(true);
    } else if (compare_extras && CompareKeys(words[i], Meow.ExtraUnsafeKeywords)) {
      AppendKeyword(episode.Extra, words[i]);
      if (IsTokenEnclosed(token)) RemoveWordFromToken(true);
    }

    #undef RemoveWordFromToken
  }
}

// =============================================================================

// Helper functions

void CRecognition::AppendKeyword(wstring& str, const wstring& keyword) {
  if (!str.empty()) str.append(L" ");
  str.append(keyword);
}

bool CRecognition::CompareKeys(const wstring& str, const vector<wstring>& keys) {
  if (!str.empty())
    for (unsigned int i = 0; i < keys.size(); i++)
      if (IsEqual(str, keys[i]))
        return true;
  return false;
}

void CRecognition::EraseUnnecessary(wstring& str) {
  Erase(str, L"The ", true);
  Erase(str, L" The", true);
}

bool CRecognition::IsEpisode(const wstring& str, CEpisode& episode) {  
  // Ep##
  if (str.length() > 2 && 
    (str.at(0) == 'E' || str.at(0) == 'e') &&
    (str.at(1) == 'P' || str.at(1) == 'p')) {
      for (unsigned int i = 2; i < str.length(); i++) {
        if (!IsNumeric(str.at(i))) {
          // Ep##v#
          if (str.at(i) == 'V' || str.at(i) == 'v') {
            for (unsigned int j = i + 1; j < str.length(); j++) {
              if (!IsNumeric(str.at(j))) return false;
            }
            episode.Number = str.substr(2, str.length() - i);
            episode.Version = str.substr(i + 1);
            return true;
          } else {
            return false;
          }
        }
      }
      episode.Number = str.substr(2);
      return true;
  // E##
  } else if (str.length() > 1 &&
    (str.at(0) == 'E' || str.at(0) == 'e')) {
      for (unsigned int i = 1; i < str.length(); i++) {
        if (!IsNumeric(str.at(i))) return false;
      }
      episode.Number = str.substr(1);
      return true;
  }
  return false;
}

bool CRecognition::IsResolution(const wstring& str) {
  // *###x###*
  if (str.length() > 6) {
    int pos = InStr(str, L"x", 0);
    if (pos > -1) {
      for (unsigned int i = 0; i < str.length(); i++) {
        if (i != pos && !IsNumeric(str.at(i))) return false;
      }
      return true;
    }
  // *###p
  } else if (str.length() > 3) {
    if (str.at(str.length() - 1) == 'p') {
      for (unsigned int i = 0; i < str.length() - 1; i++) {
        if (!IsNumeric(str.at(i))) return false;
      }
      return true;
    }
  }
  return false;
}

bool CRecognition::IsTokenEnclosed(const CToken& token) {
  return token.Encloser == '[' || token.Encloser == '(' || token.Encloser == '{';
}

void CRecognition::ReadKeyword(unsigned int uID, vector<wstring>& str) {
  wstring str_buff; 
  ReadStringTable(uID, str_buff); 
  Split(str_buff, L", ", str);
}

size_t CRecognition::TokenizeTitle(const wstring& str, const wstring& delimiters, vector<CToken>& tokens) {
  size_t index_begin = str.find_first_not_of(delimiters);
  while (index_begin != wstring::npos) {
    size_t index_end = str.find_first_of(delimiters, index_begin + 1);
    tokens.resize(tokens.size() + 1);
    if (index_end == wstring::npos) {
      tokens.back().Content = str.substr(index_begin);
      break;
    } else {
      tokens.back().Content = str.substr(index_begin, index_end - index_begin);
      if (index_begin > 0) tokens.back().Encloser = str.at(index_begin - 1);
      index_begin = str.find_first_not_of(delimiters, index_end + 1);
    }
  }
  return tokens.size();
}

bool CRecognition::ValidateEpisodeNumber(CEpisode& episode) {
  int number = ToINT(episode.Number);
  if (number > 1000) {
    if (number > 1950 && number < 2050) {
      AppendKeyword(episode.Extra, L"Year: " + episode.Number);
    }
    episode.Number.clear();
    return false;
  }
  return true;
}