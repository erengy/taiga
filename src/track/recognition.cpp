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

#include <anitomy/anitomy/anitomy.h>
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
  // Clear previous data
  episode.Clear();

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

  for (const auto& element : anitomy_instance.elements()) {
    switch (element.first) {
      case anitomy::kElementFileName:
        episode.file = element.second;
        break;
      case anitomy::kElementFileExtension:
        episode.format = element.second;
        break;
      case anitomy::kElementAnimeTitle:
        episode.title = element.second;
        break;
      case anitomy::kElementEpisodeTitle:
        episode.name = element.second;
        break;
      case anitomy::kElementReleaseGroup:
        episode.group = element.second;
        break;
      case anitomy::kElementEpisodeNumber:
        AppendString(episode.number, element.second, L"-");
        break;
      case anitomy::kElementReleaseVersion:
        episode.version = element.second;
        break;
      case anitomy::kElementVideoResolution:
        episode.resolution = element.second;
        break;
      case anitomy::kElementAudioTerm:
        AppendString(episode.audio_type, element.second, L" ");
        break;
      case anitomy::kElementVideoTerm:
        AppendString(episode.video_type, element.second, L" ");
        break;
      case anitomy::kElementFileChecksum:
        episode.checksum = element.second;
        break;
      case anitomy::kElementAnimeYear:
        episode.year = element.second;
        break;
      case anitomy::kElementAnimeType:
      case anitomy::kElementDeviceCompatibility:
      case anitomy::kElementLanguage:
      case anitomy::kElementOther:
      case anitomy::kElementSource:
      case anitomy::kElementSubtitles:
        AppendString(episode.extras, element.second, L" ");
        break;
    }
  }

  episode.normal_title = episode.title;
  Normalize(episode.normal_title);

  if (episode.normal_title.empty()) {
    LOG(LevelWarning, L"episode.clean_title is empty for file: " + episode.file);
  }

  return !episode.normal_title.empty();
}

int Engine::Identify(anime::Episode& episode, bool give_score,
                     const MatchOptions& match_options) {
  std::set<int> anime_ids;

  // Look up the title in our database
  InitializeTitles();
  LookUpTitle(episode.title, episode.normal_title, anime_ids);

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
    if (episode.number.empty() && anime_item.GetEpisodeCount() == 1)
      episode.number = L"1";
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

  if (match_options.check_anime_type) {
    // TODO: Check anime type, ignore if "ED", "OP", "PV"
  }

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
  if (!anime_item.GetEpisodeCount())
    return 1;

  int number = anime::GetEpisodeHigh(episode.number);

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

      ToLower(title);
      trigram_container_t trigrams;
      GetTrigrams(title, trigrams);
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
  ToLower(title);
  Transliterate(title);
  NormalizeUnicode(title);
  EraseUnnecessary(title);
  ErasePunctuation(title);
}

void Engine::NormalizeUnicode(std::wstring& str) const {
  static const int options =
      // NFKC normalization according to Unicode Standard Annex #15
      UTF8PROC_COMPAT | UTF8PROC_COMPOSE | UTF8PROC_STABLE |
      // Strip "default ignorable" characters, control characters, character
      // marks (accents, diaeresis)
      UTF8PROC_IGNORE | UTF8PROC_STRIPCC | UTF8PROC_STRIPMARK |
      // Map certain characters (e.g. hyphen and minus) for easier comparison
      UTF8PROC_LUMP;

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

// TODO: Rename
void Engine::EraseUnnecessary(std::wstring& str) const {
  Replace(str, L" & ", L" and ", true);

  EraseLeft(str, L"the ");
  Replace(str, L" the ", L" ", true);

  Erase(str, L"episode ");
  Replace(str, L" specials", L" special", true);
}

void Engine::Transliterate(std::wstring& str) const {
  for (size_t i = 0; i < str.size(); ++i) {
    auto& c = str[i];
    switch (c) {
      // Character equivalencies
      case L'\u00D7': c = L'x'; break;  // multiplication sign
      case L'\u00E9': c = L'e'; break;  // latin small letter e with acute
      case L'\u223C': c = L'~'; break;  // tilde operator
      case L'\u2715': c = L'x'; break;  // multiplication x
      case L'\u301C': c = L'~'; break;  // wave dash
      case L'\uFF01': c = L'!'; break;  // fullwidth exclamation mark
      case L'\uFF0F': c = L'/'; break;  // fullwidth solidus
      case L'\uFF1F': c = L'?'; break;  // fullwidth question mark
      case L'\uFF5E': c = L'~'; break;  // fullwidth tilde
      // A few common always-equivalent romanizations
      case L'\u014C': str.replace(i, 1, L"ou"); break;  // latin capital letter o with macron
      case L'\u014D': str.replace(i, 1, L"ou"); break;  // latin small letter o with macron
      case L'\u016B': str.replace(i, 1, L"uu"); break;  // latin small letter u with macron
    }
  }

  // Romanizations (Hepburn to Wapuro)
  Replace(str, L" wa ", L" ha ", true);
  Replace(str, L" e ", L" he ", true);
  Replace(str, L" o ", L" wo ", true);
}

////////////////////////////////////////////////////////////////////////////////

sorted_scores_t Engine::GetScores() const {
  return scores_;
}

int Engine::ScoreTitle(const anime::Episode& episode,
                       const std::set<int>& anime_ids) {
  scores_t trigram_results;

  auto title = episode.title;
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

}  // namespace recognition
}  // namespace track