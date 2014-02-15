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

#include "base/foreach.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/anime_episode.h"
#include "library/anime_util.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "track/media.h"
#include "track/recognition.h"

RecognitionEngine Meow;

class Token {
public:
  Token() : encloser('\0'), separator('\0'), untouched(true) {}

  std::wstring content;
  wchar_t encloser;
  wchar_t separator;
  bool untouched;
};

RecognitionEngine::RecognitionEngine() {
  ReadKeyword(audio_keywords,
      L"2CH, 5.1CH, 5.1, AAC, AC3, DTS, DTS5.1, DTS-ES, DUALAUDIO, DUAL AUDIO, "
      L"FLAC, MP3, OGG, TRUEHD5.1, VORBIS");
  ReadKeyword(video_keywords,
      L"8BIT, 10BIT, AVI, DIVX, H264, H.264, HD, HDTV, HI10P, HQ, LQ, RMVB, "
      L"SD, TS, VFR, WMV, X264, X.264, XVID");
  ReadKeyword(extra_keywords,
      L"ASS, BATCH, BD, BLURAY, BLU-RAY, COMPLETE, DIRECTOR'S CUT, DVD, DVD5, "
      L"DVD9, DVD-R2J, DVDRIP, ENG, ENGLISH, HARDSUB, PS3, R2DVD, R2J, R2JDVD, "
      L"RAW, REMASTERED, SOFTSUB, SUBBED, SUB, UNCENSORED, UNCUT, VOSTFR, "
      L"WEBCAST, WIDESCREEN, WS");
  ReadKeyword(extra_unsafe_keywords,
      L"END, FINAL, OAV, ONA, OVA");
  ReadKeyword(version_keywords,
      L"V0, V2, V3, V4");
  ReadKeyword(valid_extensions,
      L"MKV, AVI, MP4, OGM, RM, RMVB, WMV, DIVX, MOV, FLV, MPG, 3GP");
  ReadKeyword(episode_keywords,
      L"EPISODE, EP., EP, VOLUME, VOL., VOL, EPS., EPS");
  ReadKeyword(episode_prefixes,
      L"EP., EP, E, VOL., VOL, EPS., \x7B2C");
}

////////////////////////////////////////////////////////////////////////////////

anime::Item* RecognitionEngine::MatchDatabase(anime::Episode& episode,
                                              bool in_list,
                                              bool reverse,
                                              bool strict,
                                              bool check_episode,
                                              bool check_date,
                                              bool give_score) {
  // Reset scores
  foreach_(it, scores)
    it->second = 0;

  if (reverse) {
    foreach_r_(it, AnimeDatabase.items) {
      if (in_list && !it->second.IsInList())
        continue;
      if (Meow.CompareEpisode(episode, it->second, strict, check_episode,
                              check_date, give_score))
        return AnimeDatabase.FindItem(episode.anime_id);
    }
  } else {
    foreach_(it, AnimeDatabase.items) {
      if (in_list && !it->second.IsInList())
        continue;
      if (Meow.CompareEpisode(episode, it->second, strict, check_episode,
                              check_date, give_score))
        return AnimeDatabase.FindItem(episode.anime_id);
    }
  }

  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////

bool RecognitionEngine::CompareEpisode(anime::Episode& episode,
                                       const anime::Item& anime_item,
                                       bool strict,
                                       bool check_episode,
                                       bool check_date,
                                       bool give_score) {
  // Leave if title is empty
  if (episode.clean_title.empty())
    return false;

  // Leave if not yet aired
  if (check_date && !anime::IsAiredYet(anime_item))
    return false;

  bool found = false;

  // Compare with titles
  if (clean_titles[anime_item.GetId()].empty())
    UpdateCleanTitles(anime_item.GetId());
  foreach_(it, clean_titles[anime_item.GetId()]) {
    found = CompareTitle(*it, episode, anime_item, strict);
    if (found)
      break;
  }

  if (!found) {
    // Score title in case we need it later on
    if (give_score)
      ScoreTitle(episode, anime_item);
    // Leave if not found
    return false;
  }

  // Validate episode number
  if (check_episode && anime_item.GetEpisodeCount() > 0) {
    int number = anime::GetEpisodeHigh(episode.number);
    if (number > anime_item.GetEpisodeCount()) {
      // Check sequels
      auto sequel = &anime_item;
      do {
        number -= sequel->GetEpisodeCount();
        sequel = AnimeDatabase.FindSequel(sequel->GetId());
      } while (sequel && number > sequel->GetEpisodeCount());
      if (sequel) {
        episode.anime_id = sequel->GetId();
        episode.number = ToWstr(number);
        return true;
      }
      // Episode number is out of range
      return false;
    }
  }
  // Assume episode 1 if matched one-episode series
  if (episode.number.empty() && anime_item.GetEpisodeCount() == 1)
    episode.number = L"1";

  episode.anime_id = anime_item.GetId();

  return true;
}

bool RecognitionEngine::CompareTitle(const std::wstring& anime_title,
                                     anime::Episode& episode,
                                     const anime::Item& anime_item,
                                     bool strict) {
  // Compare with title + number
  if (strict && anime_item.GetEpisodeCount() == 1 && !episode.number.empty()) {
    if (IsEqual(episode.clean_title + episode.number, anime_title)) {
      episode.title += episode.number;
      episode.number.clear();
      return true;
    }
  }
  // Compare with title
  if (strict) {
    if (IsEqual(anime_title, episode.clean_title))
      return true;
  } else {
    if (InStr(anime_title, episode.clean_title, 0, true) > -1)
      return true;
  }

  return false;
}

std::multimap<int, int, std::greater<int>> RecognitionEngine::GetScores() {
  std::multimap<int, int, std::greater<int>> reverse_map;

  foreach_(it, scores) {
    if (it->second == 0)
      continue;
    reverse_map.insert(std::pair<int, int>(it->second, it->first));
  }

  return reverse_map;
}

bool RecognitionEngine::ScoreTitle(const anime::Episode& episode,
                                   const anime::Item& anime_item) {
  const std::wstring& episode_title = episode.clean_title;
  const std::wstring& anime_title = clean_titles[anime_item.GetId()].front();

  const int score_bonus_small = 1;
  const int score_bonus_big = 5;
  const int score_min = std::abs(static_cast<int>(episode_title.length()) -
                                 static_cast<int>(anime_title.length()));
  const int score_max = episode_title.length() + anime_title.length();

  int score = score_max;

  score -= LevenshteinDistance(episode_title, anime_title);

  score += LongestCommonSubsequenceLength(episode_title, anime_title) * 2;
  score += LongestCommonSubstringLength(episode_title, anime_title) * 4;

  if (score <= score_min)
    return false;

  if (anime_item.IsInList()) {
    score += score_bonus_big;
    switch (anime_item.GetMyStatus()) {
      case anime::kWatching:
      case anime::kPlanToWatch:
        score += score_bonus_small;
        break;
    }
  }
  switch (anime_item.GetType()) {
    case anime::kTv:
      score += score_bonus_small;
      break;
  }
  if (!episode.year.empty()) {
    if (anime_item.GetDateStart().year == ToInt(episode.year)) {
      score += score_bonus_big;
    }
  }

  if (score > score_min) {
    scores[anime_item.GetId()] = score;
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

bool RecognitionEngine::ExamineTitle(std::wstring title,
                                     anime::Episode& episode,
                                     bool examine_inside,
                                     bool examine_outside,
                                     bool examine_number,
                                     bool check_extras,
                                     bool check_extension) {
  // Clear previous data
  episode.Clear();

  if (title.empty())
    return false;

  // Remove zero width space character
  EraseChars(title, L"\u200B");  // TEMP

  // Retrieve file name from full path
  if (title.length() > 2 && title.at(1) == ':' && title.at(2) == '\\') {
    episode.folder = GetPathOnly(title);
    title = GetFileName(title);
  }
  episode.file = title;

  // Ignore if the file is outside of root folders
  if (Settings.GetBool(taiga::kSync_Update_OutOfRoot))
    if (!episode.folder.empty() && !Settings.root_folders.empty())
      if (!anime::IsInsideRootFolders(episode.folder))
        return false;

  // Check and trim file extension
  std::wstring extension = GetFileExtension(title);
  if (!extension.empty() &&
      extension.length() < title.length() &&
      extension.length() <= 5) {
    if (IsAlphanumeric(extension) &&
        CheckFileExtension(extension, valid_extensions)) {
      episode.format = ToUpper_Copy(extension);
      title.resize(title.length() - extension.length() - 1);
    } else {
      if (IsNumeric(extension)) {
        std::wstring temp = title.substr(0, title.length() - extension.length());
        foreach_(it, episode_keywords) {
          if (temp.length() >= it->length() &&
              IsEqual(CharRight(temp, it->length()), *it)) {
            title.resize(title.length() - extension.length() - it->length() - 1);
            episode.number = extension;
            break;
          }
        }
      }
      if (check_extension && episode.number.empty())
        return false;
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Here we are, entering the world of tokens. Each token has four properties:
  // * content: Self-explanatory
  // * encloser: Can be empty or one of these characters: [](){}
  // * separator: The most common non-alphanumeric character - usually a space
  //   or an underscore
  // * untouched: All tokens start out as untouched but lose this property when
  //   some keyword within is recognized and erased.

  // Tokenize
  std::vector<Token> tokens;
  tokens.reserve(4);
  TokenizeTitle(title, L"[](){}", tokens);
  if (tokens.empty())
    return false;
  title.clear();

  // Examine tokens
  foreach_(token, tokens) {
    if (IsTokenEnclosed(*token)) {
      if (examine_inside)
        ExamineToken(*token, episode, check_extras);
    } else {
      if (examine_outside)
        ExamineToken(*token, episode, check_extras);
    }
  }

  // Tidy up tokens
  for (size_t i = 1; i < tokens.size() - 1; i++) {
    // Combine remaining tokens that are enclosed with parentheses - this is
    // especially useful for titles that include a year value and some other cases
    if (tokens[i - 1].untouched == false ||
        tokens[i].untouched == false ||
        IsTokenEnclosed(tokens[i - 1]) == true ||
        tokens[i].encloser != '(' ||
        tokens[i - 1].content.length() < 2) {
      continue;
    }
    tokens[i - 1].content += L"(" + tokens[i].content + L")";
    if (IsTokenEnclosed(tokens[i + 1]) == false) {
      tokens[i - 1].content += tokens[i + 1].content;
      if (tokens[i - 1].separator == '\0')
        tokens[i - 1].separator = tokens[i + 1].separator;
      tokens.erase(tokens.begin() + i + 1);
    }
    tokens.erase(tokens.begin() + i);
    i = 0;
  }
  for (size_t i = 0; i < tokens.size(); i++) {
    // Trim separator character from each side of the token
    wchar_t trim_char[] = {tokens[i].separator, '\0'};
    Trim(tokens[i].content, trim_char);
    // Tokens that are too short are now garbage, so we take them out
    if (tokens[i].content.length() < 2 && !IsNumeric(tokens[i].content)) {
      tokens.erase(tokens.begin() + i);
      i--;
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Now we apply some logic to decide on the title and the group name

  int group_index = -1;
  int title_index = -1;
  std::vector<int> group_vector;
  std::vector<int> title_vector;
  for (size_t i = 0; i < tokens.size(); i++) {
    if (IsTokenEnclosed(tokens[i])) {
      group_vector.push_back(i);
    } else {
      title_vector.push_back(i);
    }
  }

  // Choose the first free token as the title, if there is one
  if (title_vector.size() > 0) {
    title_index = title_vector[0];
    title_vector.erase(title_vector.begin());
  } else {
    // Choose the second enclosed token as the title, if there is more than one
    // (it is more probable that the group name comes before the title)
    if (group_vector.size() > 1) {
      title_index = group_vector[1];
      group_vector.erase(group_vector.begin() + 1);
    // Choose the first enclosed token as the title, if there is one
    // (which means that there is no group name available)
    } else if (group_vector.size() > 0) {
      title_index = group_vector[0];
      group_vector.erase(group_vector.begin());
    }
  }

  // Choose the first enclosed untouched token as the group name
  for (size_t i = 0; i < group_vector.size(); i++) {
    // Here we assume that group names are never enclosed with other keywords
    if (tokens[group_vector[i]].untouched) {
      group_index = group_vector[i];
      group_vector.erase(group_vector.begin() + i);
      break;
    }
  }
  // Group name might not be enclosed at all
  // This is a special case for THORA releases, where the group name is at the end
  if (group_index == -1) {
    if (title_vector.size() > 0) {
      group_index = title_vector.back();
      title_vector.erase(title_vector.end() - 1);
    }
  }

  // Do we have a title?
  if (title_index > -1) {
    // Replace the separator with a space character
    ReplaceChar(tokens[title_index].content, tokens[title_index].separator, ' ');
    // Do some clean-up
    Trim(tokens[title_index].content, L" -");
    // Set the title
    title = tokens[title_index].content;
    tokens[title_index].content.clear();
    tokens[title_index].untouched = false;
  }

  // Do we have a group name?
  if (group_index > -1) {
    // We don't want to lose any character if the token is enclosed, because
    // those characters can be a part of the group name itself
    if (!IsTokenEnclosed(tokens[group_index])) {
      // Replace the separator with a space character
      ReplaceChar(tokens[group_index].content, tokens[group_index].separator, ' ');
      // Do some clean-up
      Trim(tokens[group_index].content, L" -");
    }
    // Set the group name
    episode.group = tokens[group_index].content;
    // We don't clear token content here, becuse we'll be checking it for
    // episode number later on
  }

  //////////////////////////////////////////////////////////////////////////////
  // Get episode number and version, if available
  
  if (examine_number) {
    // Check remaining tokens first
    foreach_(token, tokens) {
      if (IsEpisodeFormat(token->content, episode, token->separator)) {
        token->untouched = false;
        break;
      }
    }

    // Check title
    if (episode.number.empty()) {
      // Split into words
      std::vector<std::wstring> words;
      words.reserve(4);
      Tokenize(title, L" ", words);
      if (words.empty())
        return false;
      title.clear();
      int number_index = -1;

      // Check for episode number format, starting with the second word
      for (size_t i = 1; i < words.size(); i++) {
        if (IsEpisodeFormat(words[i], episode)) {
          number_index = static_cast<int>(i);
          break;
        }
      }

      // Set the first valid numeric token as episode number
      if (episode.number.empty()) {
        foreach_(token, tokens) {
          if (IsNumeric(token->content)) {
            episode.number = token->content;
            if (ValidateEpisodeNumber(episode)) {
              token->untouched = false;
              break;
            }
          }
        }
      }

      // Set the lastmost number that follows a '-'
      if (episode.number.empty() && words.size() > 2) {
        for (size_t i = words.size() - 2; i > 0; i--) {
          if (words[i] == L"-" && IsNumeric(words[i + 1])) {
            episode.number = words[i + 1];
            if (ValidateEpisodeNumber(episode)) {
              number_index = static_cast<int>(i) + 1;
              break;
            }
          }
        }
      }

      // Set the lastmost number as a last resort
      if (episode.number.empty()) {
        for (size_t i = words.size() - 1; i > 0; i--) {
          if (IsNumeric(words[i])) {
            episode.number = words[i];
            if (ValidateEpisodeNumber(episode)) {
              // Discard and give up if movie or season number (episode numbers
              // cannot precede them)
              if (i > 1 &&
                  (IsEqual(words[i - 1], L"Season") ||
                   IsEqual(words[i - 1], L"Movie")) &&
                  !IsCountingWord(words[i - 2])) {
                episode.number.clear();
                number_index = -1;
                break;
              }

              number_index = static_cast<int>(i);
              break;
            }
          }
        }
      }
  
      // Build title and name
      for (int i = 0; i < static_cast<int>(words.size()); i++) {
        if (number_index == -1 || i < number_index) {
          // Ignore episode keywords
          if (i == number_index - 1 && CompareKeys(words[i], episode_keywords))
            continue;
          AppendKeyword(title, words[i]);
        } else if (i > number_index) {
          AppendKeyword(episode.name, words[i]);
        }
      }

      // Clean up
      TrimRight(title, L" -");
      TrimLeft(episode.name, L" -");
      if (StartsWith(episode.name, L"'") && EndsWith(episode.name, L"'"))
        Trim(episode.name, L"'");
      TrimLeft(episode.name, L"\u300C");  // Japanese left quotation mark
      TrimRight(episode.name, L"\u300D");  // Japanese right quotation mark
    }
  }

  //////////////////////////////////////////////////////////////////////////////

  // Check if the group token is still untouched
  if (group_index > -1 && !tokens[group_index].untouched) {
    episode.group.clear();
    foreach_(token, tokens) {
      // Set the first available untouched token as group name
      if (!token->content.empty() && token->untouched) {
        episode.group = token->content;
        break;
      }
    }
  }
  // Set episode name as group name if necessary
  if (episode.group.empty() && episode.name.length() > 2) {
    if (StartsWith(episode.name, L"(") && EndsWith(episode.name, L")")) {
      episode.group = episode.name.substr(1, episode.name.length() - 2);
      episode.name.clear();
    }
  }

  // Examine remaining tokens once more
  foreach_(token, tokens)
    if (!token->content.empty())
      ExamineToken(*token, episode, true);

  //////////////////////////////////////////////////////////////////////////////

  // Set the final title, hopefully name of the anime
  episode.title = title;
  episode.clean_title = title;
  CleanTitle(episode.clean_title);

  return !title.empty();
}

////////////////////////////////////////////////////////////////////////////////

void RecognitionEngine::ExamineToken(Token& token, anime::Episode& episode,
                                     bool compare_extras) {
  // Split into words. The most common non-alphanumeric character is the 
  // separator.
  std::vector<std::wstring> words;
  token.separator = GetMostCommonCharacter(token.content);
  Split(token.content, std::wstring(1, token.separator), words);

  // Revert if there are words that are too short. This prevents splitting some
  // group names (e.g. "m.3.3.w") and keywords (e.g. "H.264").
  if (IsTokenEnclosed(token)) {
    foreach_(word, words) {
      if (word->length() == 1) {
        words.clear();
        words.push_back(token.content);
        break;
      }
    }
  }

  // Compare with keywords
  foreach_(word, words) {
    Trim(*word);
    if (word->empty())
      continue;
    #define RemoveWordFromToken(b) { \
      Erase(token.content, *word, b); token.untouched = false; }
    
    // Checksum
    if (episode.checksum.empty() && word->length() == 8 && IsHex(*word)) {
      episode.checksum = *word;
      RemoveWordFromToken(false);
    // Video resolution
    } else if (episode.resolution.empty() && IsResolution(*word)) {
      episode.resolution = *word;
      RemoveWordFromToken(false);
    // Video info
    } else if (CompareKeys(*word, video_keywords)) {
      AppendKeyword(episode.video_type, *word);
      RemoveWordFromToken(true);
    // Audio info
    } else if (CompareKeys(*word, audio_keywords)) {
      AppendKeyword(episode.audio_type, *word);
      RemoveWordFromToken(true);
    // Version
    } else if (episode.version.empty() && CompareKeys(*word, version_keywords)) {
      episode.version.push_back(word->at(word->length() - 1));
      RemoveWordFromToken(true);
    // Extras
    } else if (compare_extras && CompareKeys(*word, extra_keywords)) {
      AppendKeyword(episode.extras, *word);
      RemoveWordFromToken(true);
    } else if (compare_extras && CompareKeys(*word, extra_unsafe_keywords)) {
      AppendKeyword(episode.extras, *word);
      if (IsTokenEnclosed(token))
        RemoveWordFromToken(true);
    }

    #undef RemoveWordFromToken
  }
}

////////////////////////////////////////////////////////////////////////////////

// Helper functions

void RecognitionEngine::AppendKeyword(std::wstring& str,
                                      const std::wstring& keyword) {
  AppendString(str, keyword, L" ");
}

bool RecognitionEngine::CompareKeys(const std::wstring& str,
                                    const std::vector<std::wstring>& keys) {
  if (!str.empty())
    foreach_(key, keys)
      if (IsEqual(str, *key))
        return true;

  return false;
}

void RecognitionEngine::CleanTitle(std::wstring& title) {
  if (title.empty())
    return;

  EraseUnnecessary(title);
  TransliterateSpecial(title);
  ErasePunctuation(title, true);
}

void RecognitionEngine::UpdateCleanTitles(int anime_id) {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  clean_titles[anime_id].clear();

  // Main title
  clean_titles[anime_id].push_back(anime_item->GetTitle());
  CleanTitle(clean_titles[anime_id].back());

  // English title
  if (!anime_item->GetEnglishTitle().empty()) {
    clean_titles[anime_id].push_back(anime_item->GetEnglishTitle());
    CleanTitle(clean_titles[anime_id].back());
  }

  // Synonyms
  if (!anime_item->GetUserSynonyms().empty()) {
    foreach_(it, anime_item->GetUserSynonyms()) {
      clean_titles[anime_id].push_back(*it);
      CleanTitle(clean_titles[anime_id].back());
    }
  }
  if (!anime_item->GetSynonyms().empty()) {
    auto synonyms = anime_item->GetSynonyms();
    foreach_(it, synonyms) {
      clean_titles[anime_id].push_back(*it);
      CleanTitle(clean_titles[anime_id].back());
    }
  }
}

void RecognitionEngine::EraseUnnecessary(std::wstring& str) {
  EraseLeft(str, L"the ", true);
  Replace(str, L" the ", L" ", false, true);
  Erase(str, L"episode ", true);
  Erase(str, L" ep.", true);
  Replace(str, L" specials", L" special", false, true);
}

// TODO: make faster
void RecognitionEngine::TransliterateSpecial(std::wstring& str) {
  // Character equivalencies
  ReplaceChar(str, L'\u00E9', L'e');  // small e acute accent
  ReplaceChar(str, L'\uFF0F', L'/');  // unicode slash
  ReplaceChar(str, L'\uFF5E', L'~');  // unicode tilde
  ReplaceChar(str, L'\u223C', L'~');  // unicode tilde 2
  ReplaceChar(str, L'\u301C', L'~');  // unicode tilde 3
  ReplaceChar(str, L'\uFF1F', L'?');  // unicode question mark
  ReplaceChar(str, L'\uFF01', L'!');  // unicode exclamation point
  ReplaceChar(str, L'\u00D7', L'x');  // multiplication symbol
  ReplaceChar(str, L'\u2715', L'x');  // multiplication symbol 2

  // A few common always-equivalent romanizations
  Replace(str, L"\u014C", L"Ou");  // O macron
  Replace(str, L"\u014D", L"ou");  // o macron
  Replace(str, L"\u016B", L"uu");  // u macron
  Replace(str, L" wa ", L" ha ");  // hepburn to wapuro
  Replace(str, L" e ", L" he ");  // hepburn to wapuro
  Replace(str, L" o ", L" wo ");  // hepburn to wapuro

  // Abbreviations
  Replace(str, L" & ", L" and ", true, false);
}

bool RecognitionEngine::IsEpisodeFormat(const std::wstring& str,
                                        anime::Episode& episode,
                                        const wchar_t separator) {
  unsigned int numstart, i, j;

  // Find first number
  for (numstart = 0; numstart < str.length() && !IsNumeric(str.at(numstart)); numstart++);
  if (numstart == str.length())
    return false;

  // Check for episode prefix
  if (numstart > 0)
    if (!CompareKeys(str.substr(0, numstart), episode_prefixes))
      return false;

  for (i = numstart + 1; i < str.length(); i++) {
    if (!IsNumeric(str.at(i))) {
      // *#-#*
      if (str.at(i) == '-' || str.at(i) == '&') {
        if (i == str.length() - 1 || !IsNumeric(str.at(i + 1)))
          return false;
        for (j = i + 1; j < str.length() && IsNumeric(str.at(j)); j++);
        episode.number = str.substr(numstart, j - numstart);
        // *#-#*v#
        if (j < str.length() - 1 && (str.at(j) == 'v' || str.at(j) =='V')) {
          numstart = i + 1;
          continue;
        } else {
          return true;
        }

      // v#
      } else if (str.at(i) == 'v' || str.at(i) == 'V') {
        if (episode.number.empty()) {
          if (i == str.length() - 1 || !IsNumeric(str.at(i + 1))) return false;
          episode.number = str.substr(numstart, i - numstart);
        }
        episode.version = str.substr(i + 1);
        return true;
      
      // *# of #*
      } else if (str.at(i) == separator) {
        if (str.length() < i + 5)
          return false;
        if (str.at(i + 1) == 'o' &&
            str.at(i + 2) == 'f' &&
            str.at(i + 3) == separator) {
          episode.number = str.substr(numstart, i - numstart);
          return true;
        }

      // *#x#*
      } else if (str.at(i) == 'x') {
        if (i == str.length() - 1 || !IsNumeric(str.at(i + 1)))
          return false;
        for (j = i + 1; j < str.length() && IsNumeric(str.at(j)); j++);
        episode.number = str.substr(i + 1, j - i - 1);
        if (ToInt(episode.number) < 100) {
          return true;
        } else {
          episode.number.clear();
          return false;
        }

      // Japanese counter
      } else if (str.at(i) == L'\u8A71') {
        episode.number = str.substr(numstart, i - 1);
        return true;

      } else {
        episode.number.clear();
        return false;
      }
    }
  }

  // [prefix]#*
  if (numstart > 0 && episode.number.empty()) {
    episode.number = str.substr(numstart, str.length() - numstart);
    if (!ValidateEpisodeNumber(episode))
      return false;
    return true;
  }

  // *#
  return false;
}

bool RecognitionEngine::IsResolution(const std::wstring& str) {
  return anime::TranslateResolution(str, true) > 0;
}

bool RecognitionEngine::IsCountingWord(const std::wstring& str) {
  if (str.length() > 2) {
    if (EndsWith(str, L"th") || EndsWith(str, L"nd") || EndsWith(str, L"rd") || EndsWith(str, L"st") ||
        EndsWith(str, L"TH") || EndsWith(str, L"ND") || EndsWith(str, L"RD") || EndsWith(str, L"ST")) {
      if (IsNumeric(str.substr(0, str.length() - 2)) ||
          IsEqual(str, L"FIRST") ||
          IsEqual(str, L"SECOND") ||
          IsEqual(str, L"THIRD") ||
          IsEqual(str, L"FOURTH") ||
          IsEqual(str, L"FIFTH")) {
        return true;
      }
    }
  }

  return false;
}

bool RecognitionEngine::IsTokenEnclosed(const Token& token) {
  return token.encloser == '[' ||
         token.encloser == '(' ||
         token.encloser == '{';
}

void RecognitionEngine::ReadKeyword(std::vector<std::wstring>& output,
                                    const std::wstring& input) {
  Split(input, L", ", output);
}

size_t RecognitionEngine::TokenizeTitle(const std::wstring& str,
                                        const std::wstring& delimiters,
                                        std::vector<Token>& tokens) {
  size_t index_begin = str.find_first_not_of(delimiters);

  while (index_begin != std::wstring::npos) {
    size_t index_end = str.find_first_of(delimiters, index_begin + 1);
    tokens.resize(tokens.size() + 1);
    if (index_end == std::wstring::npos) {
      tokens.back().content = str.substr(index_begin);
      break;
    } else {
      tokens.back().content = str.substr(index_begin, index_end - index_begin);
      if (index_begin > 0)
        tokens.back().encloser = str.at(index_begin - 1);
      index_begin = str.find_first_not_of(delimiters, index_end + 1);
    }
  }

  return tokens.size();
}

bool RecognitionEngine::ValidateEpisodeNumber(anime::Episode& episode) {
  int number = ToInt(episode.number);

  if (number <= 0 || number > 1000) {
    if (number > 1950 && number < 2050)
      episode.year = episode.number;
    episode.number.clear();
    return false;
  }

  return true;
}