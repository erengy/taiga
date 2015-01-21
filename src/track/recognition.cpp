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

#include <algorithm>
#include <set>

#include <anitomy/anitomy/anitomy.h>
#include <anitomy/anitomy/keyword.h>
#include <libmojibake/mojibake.h>

#include "base/log.h"
#include "base/string.h"
#include "library/anime.h"
#include "library/anime_db.h"
#include "library/anime_episode.h"
#include "library/anime_util.h"
#include "taiga/settings.h"
#include "track/recognition.h"

track::recognition::Engine Meow;

namespace track {
namespace recognition {

MatchOptions::MatchOptions()
    : allow_sequels(false),
      check_airing_date(false),
      check_anime_type(false),
      validate_episode_number(false) {
}

////////////////////////////////////////////////////////////////////////////////

bool Engine::Parse(std::wstring title, anime::Episode& episode) const {
  episode.Clear();  // Clear previous data

  if (title.empty())
    return false;

  // Retrieve filename from full path
  if (title.length() > 2 && title.at(1) == ':' && title.at(2) == '\\') {
    episode.folder = GetPathOnly(title);
    title = GetFileName(title);
  }

  anitomy::Anitomy anitomy_instance;

  if (!anitomy_instance.Parse(title))
    return false;

  episode.set_elements(anitomy_instance.elements());

  episode.normal_title = episode.anime_title();
  Normalize(episode.normal_title);

  if (episode.normal_title.empty())
    LOG(LevelWarning, L"episode.clean_title is empty for file: " +
                      episode.file_name());

  return !episode.normal_title.empty();
}

int Engine::Identify(anime::Episode& episode, bool give_score,
                     const MatchOptions& match_options) {
  std::set<int> anime_ids;

  // Look up the title in our database
  InitializeTitles();
  LookUpTitle(episode.anime_title(), episode.normal_title, anime_ids);

  // Validate IDs
  for (auto it = anime_ids.begin(); it != anime_ids.end(); ) {
    if (!ValidateOptions(episode, *it, match_options)) {
      it = anime_ids.erase(it);
    } else {
      ++it;
    }
  }

  // Figure out which ID is the one we're looking for
  if (anime_ids.size() == 1) {
    episode.anime_id = *anime_ids.begin();
  } else if (anime_ids.size() > 1) {
    episode.anime_id = ScoreTitle(episode, anime_ids);
  } else if (anime_ids.empty() && give_score) {
    episode.anime_id = ScoreTitle(episode, anime_ids);
  }

  // Post-processing
  if (anime::IsValidId(episode.anime_id)) {
    const anime::Item& anime_item = *AnimeDatabase.FindItem(episode.anime_id);
    // Assume episode 1, if matched a single-episode anime
    if (!episode.episode_number() && anime_item.GetEpisodeCount() == 1)
      episode.set_episode_number(1);
    // TODO: check for sequels
  }

  return episode.anime_id;
}

////////////////////////////////////////////////////////////////////////////////

bool Engine::ValidateOptions(anime::Episode& episode, int anime_id,
                             const MatchOptions& match_options) const {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (!anime_item)
    return false;

  if (match_options.check_airing_date)
    if (!anime::IsAiredYet(*anime_item))
      return false;

  if (match_options.check_anime_type)
    if (!IsValidAnimeType(episode))
      return false;

  if (match_options.validate_episode_number) {
    int result = ValidateEpisodeNumber(episode, *anime_item);
    if (result == 0)
      return false;
    if (result == -1 && !match_options.allow_sequels)
      return false;
  }

  return true;
}

int Engine::ValidateEpisodeNumber(anime::Episode& episode,
                                  const anime::Item& anime_item) const {
  if (!anime::IsValidEpisodeCount(anime_item.GetEpisodeCount()))
    return 1;

  int number = anime::GetEpisodeHigh(episode);

  if (number > anime_item.GetEpisodeCount()) {
    // Check sequels
    auto sequel = &anime_item;
    do {
      number -= sequel->GetEpisodeCount();
      sequel = AnimeDatabase.FindSequel(sequel->GetId());
    } while (sequel && number > sequel->GetEpisodeCount());
    if (sequel) {
      //episode.anime_id = sequel->GetId();
      //episode.number = ToWstr(number);
      return -1;
    }

    return 0;  // Episode number is out of range
  }

  // TODO: Clarify return values
  return 1;
}

////////////////////////////////////////////////////////////////////////////////

void Engine::InitializeTitles() {
  static bool initialized = false;

  if (initialized)
    return;
  initialized = true;

  for (const auto& it : AnimeDatabase.items)
    UpdateTitles(it.second);
}

void Engine::UpdateTitles(const anime::Item& anime_item) {
  auto update_title = [&](std::wstring title,
                          title_container_t& titles,
                          title_container_t& normal_titles) {
    if (!title.empty()) {
      titles[title.c_str()].insert(anime_item.GetId());

      trigram_container_t trigrams;
      GetTrigrams(ToLower_Copy(title), trigrams);
      trigrams_[anime_item.GetId()].push_back(trigrams);

      Normalize(title);
      normal_titles[title.c_str()].insert(anime_item.GetId());
    }
  };

  update_title(anime_item.GetTitle(), titles_.main, normal_titles_.main);
  update_title(anime_item.GetEnglishTitle(), titles_.main, normal_titles_.main);

  for (const auto& synonym : anime_item.GetSynonyms()) {
    update_title(synonym, titles_.alternative, normal_titles_.alternative);
  }
  for (const auto& synonym : anime_item.GetUserSynonyms()) {
    update_title(synonym, titles_.user, normal_titles_.user);
  }
}

int Engine::LookUpTitle(const std::wstring& title,
                        const std::wstring& normal_title,
                        std::set<int>& anime_ids) const {
  int anime_id = anime::ID_UNKNOWN;

  auto find_title = [&](const std::wstring& title,
                        const title_container_t& container) {
    if (!anime::IsValidId(anime_id)) {
      auto it = container.find(title.c_str());
      if (it != container.end()) {
        anime_ids.insert(it->second.begin(), it->second.end());
        if (anime_ids.size() == 1)
          anime_id = *anime_ids.begin();
      }
    }
  };

  find_title(title, titles_.user);
  find_title(title, titles_.main);
  find_title(title, titles_.alternative);

  if (anime_ids.size() == 1)
    return anime_id;

  find_title(normal_title, normal_titles_.user);
  find_title(normal_title, normal_titles_.main);
  find_title(normal_title, normal_titles_.alternative);

  return anime_id;
}

////////////////////////////////////////////////////////////////////////////////

void Engine::Normalize(std::wstring& title) const {
  ConvertRomanNumbers(title);
  Transliterate(title);
  NormalizeUnicode(title);  // Title is lower case after this point, due to UTF8PROC_CASEFOLD
  ConvertOrdinalNumbers(title);
  ConvertSeasonNumbers(title);
  EraseUnnecessary(title);
  ErasePunctuation(title);
}

void Engine::ConvertOrdinalNumbers(std::wstring& str) const {
  static const std::vector<std::pair<std::wstring, std::wstring>> ordinals{
    {L"1st", L"first"}, {L"2nd", L"second"}, {L"3rd", L"third"},
    {L"4th", L"fourth"}, {L"5th", L"fifth"}, {L"6th", L"sixth"},
    {L"7th", L"seventh"}, {L"8th", L"eighth"}, {L"9th", L"ninth"},
  };

  for (const auto& ordinal : ordinals)
    ReplaceString(str, 0, ordinal.second, ordinal.first, true, true);
}

void Engine::ConvertRomanNumbers(std::wstring& str) const {
  // We skip 1 and 10 to avoid matching "I" and "X", as they're unlikely to be
  // used as Roman numerals. Any number above "XIII" is rarely used in anime
  // titles, which is why we don't need an actual Roman-to-Arabic number
  // conversion algorithm.
  static const std::vector<std::pair<std::wstring, std::wstring>> numerals{
    {L"2", L"II"}, {L"3", L"III"}, {L"4", L"IV"}, {L"5", L"V"},
    {L"6", L"VI"}, {L"7", L"VII"}, {L"8", L"VIII"}, {L"9", L"IX"},
    {L"11", L"XI"}, {L"12", L"XII"}, {L"13", L"XIII"},
  };

  for (const auto& numeral : numerals)
    ReplaceString(str, 0, numeral.second, numeral.first, true, true);
}

void Engine::ConvertSeasonNumbers(std::wstring& str) const {
  // This works considerably faster than regular expressions.
  typedef std::vector<std::wstring> season_t;
  static const std::vector<std::pair<std::wstring, season_t>> values{
    {L"1", {L"1st season", L"season 1", L"s1"}},
    {L"2", {L"2nd season", L"season 2", L"s2"}},
    {L"3", {L"3rd season", L"season 3", L"s3"}},
    {L"4", {L"4th season", L"season 4", L"s4"}},
    {L"5", {L"5th season", L"season 5", L"s5"}},
    {L"6", {L"6th season", L"season 6", L"s6"}},
  };

  for (const auto& value : values)
    for (const auto& season : value.second)
      ReplaceString(str, 0, season, value.first, true, true);
}

void Engine::Transliterate(std::wstring& str) const {
  for (size_t i = 0; i < str.size(); ++i) {
    auto& c = str[i];
    switch (c) {
      // Character equivalencies
      case L'@': c = L'a'; break;  // e.g. "iDOLM@STER" (doesn't make a difference for "GJ-bu@" or "Sasami-san@Ganbaranai")
      case L'\u00D7': c = L'x'; break;  // multiplication sign (e.g. "Tasogare Otome x Amnesia")
      // A few common always-equivalent romanizations
      case L'\u014C': str.replace(i, 1, L"ou"); break;  // latin capital letter o with macron
      case L'\u014D': str.replace(i, 1, L"ou"); break;  // latin small letter o with macron
      case L'\u016B': str.replace(i, 1, L"uu"); break;  // latin small letter u with macron
    }
  }

  // Romanizations (Hepburn to Wapuro)
  ReplaceString(str, 0, L"wa", L"ha", true, true);
  ReplaceString(str, 0, L"e", L"he", true, true);
  ReplaceString(str, 0, L"o", L"wo", true, true);
}

void Engine::NormalizeUnicode(std::wstring& str) const {
  static const int options =
      // NFKC normalization according to Unicode Standard Annex #15
      UTF8PROC_COMPAT | UTF8PROC_COMPOSE | UTF8PROC_STABLE |
      // Strip "default ignorable" characters, control characters, character
      // marks (accents, diaeresis)
      UTF8PROC_IGNORE | UTF8PROC_STRIPCC | UTF8PROC_STRIPMARK |
      // Map certain characters (e.g. hyphen and minus) for easier comparison
      UTF8PROC_LUMP |
      // Perform unicode case folding for case-insensitive comparison
      UTF8PROC_CASEFOLD;

  char* buffer = nullptr;
  std::string temp = WstrToStr(str);

  int length = utf8proc_map(
      reinterpret_cast<const uint8_t*>(temp.data()), temp.length(),
      reinterpret_cast<uint8_t**>(&buffer), options);

  if (length >= 0) {
    temp.assign(buffer, length);
    str = StrToWstr(temp);
  }

  if (buffer)
    free(buffer);
}

// TODO: Rename
void Engine::EraseUnnecessary(std::wstring& str) const {
  ReplaceString(str, 0, L"&", L"and", true, true);
  ReplaceString(str, 0, L"the", L"", true, true);
  ReplaceString(str, 0, L"episode", L"", true, true);
  ReplaceString(str, 0, L"specials", L"special", true, true);
}

void Engine::ErasePunctuation(std::wstring& str) const {
  auto it = std::remove_if(str.begin(), str.end(),
      [](wchar_t c) -> bool {
        // Control codes, white-space and punctuation characters
        if (c <= 255 && !isalnum(c))
          return true;
        // Unicode stars, hearts, notes, etc. (0x2000-0x2767)
        if (c > 8192 && c < 10087)
          return true;
        // Valid character
        return false;
      });

  if (it != str.end())
    str.resize(std::distance(str.begin(), it));
}

////////////////////////////////////////////////////////////////////////////////

sorted_scores_t Engine::GetScores() const {
  return scores_;
}

int Engine::ScoreTitle(const anime::Episode& episode,
                       const std::set<int>& anime_ids) {
  scores_t trigram_results;

  auto title = episode.anime_title();
  ToLower(title);

  trigram_container_t t1;
  GetTrigrams(title, t1);

  for (const auto& it : AnimeDatabase.items) {
    const int anime_id = it.first;

    if (!anime_ids.empty() && anime_ids.find(anime_id) == anime_ids.end())
      continue;

    for (const auto& t2 : trigrams_[anime_id]) {
      double result = CompareTrigrams(t1, t2);
      if (result > 0.1)
        trigram_results[anime_id] = max(trigram_results[anime_id], result);
    }
  }

  return ScoreTitle(title, episode, trigram_results);
}

int Engine::ScoreTitle(const std::wstring& str, const anime::Episode& episode,
                       const scores_t& trigram_results) {
  scores_t levenshtein;
  scores_t jaro_winkler;
  scores_t subsequence;
  scores_t substring;
  scores_t custom;

  for (const auto& it : trigram_results) {
    int id = it.first;

    std::vector<std::wstring> titles;
    anime::GetAllTitles(id, titles);

    for (auto& title : titles) {
      ToLower(title);

      auto len = static_cast<double>(max(title.size(), str.size()));

      levenshtein[id] = max(levenshtein[id], LevenshteinDistance(title, str));
      jaro_winkler[id] = max(jaro_winkler[id], JaroWinklerDistance(title, str));
      subsequence[id] = max(subsequence[id], LongestCommonSubsequenceLength(title, str) / len);
      substring[id] = max(substring[id], LongestCommonSubstringLength(title, str) / len);

      auto val_temp = (title.length() - str.length()) / len;
      if (StartsWith(title, str)) {
        custom[id] = max(custom[id], 1.0 - val_temp);
      } else if (InStr(title, str) > -1) {
        custom[id] = max(custom[id], 1.0 - (val_temp * 0.75));
      }
    }
  }

  scores_.clear();
  for (const auto& it : trigram_results) {
    int id = it.first;
    double value = ((1.0 * it.second +
                     0.4 * levenshtein[id] +
                     0.5 * subsequence[id] +
                     0.6 * substring[id] +
                     2.0 * jaro_winkler[id]) / 4.5) + custom[id];
    if (value >= 0.5)
      scores_.push_back(std::make_pair(id, value));
  }
  std::stable_sort(scores_.begin(), scores_.end(),
      [&](const std::pair<int, double>& a,
          const std::pair<int, double>& b) -> bool {
        return a.second > b.second;
      });

  double score_1st = scores_.size() > 0 ? scores_.at(0).second : 0.0;
  double score_2nd = scores_.size() > 1 ? scores_.at(1).second : 0.0;

  if (score_1st > 1.0 && score_1st != score_2nd)
    return scores_.front().first;

  return anime::ID_UNKNOWN;
}

////////////////////////////////////////////////////////////////////////////////

bool Engine::IsValidAnimeType(const anime::Episode& episode) const {
  static const std::set<std::wstring> invalid_anime_types{
    L"ED", L"ENDING", L"NCED", L"NCOP", L"OP", L"OPENING", L"PV"
  };

  auto anime_type = episode.anime_type();
  if (!anime_type.empty())
    if (invalid_anime_types.count(anime_type))
      return false;

  return true;
}

bool Engine::IsValidFileExtension(std::wstring extension) const {
  if (extension.empty())
    return false;

  extension = anitomy::keyword_manager.Normalize(extension);
  return anitomy::keyword_manager.Find(anitomy::kElementFileExtension,
                                       extension);
}

}  // namespace recognition
}  // namespace track