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
    : check_airing_date(false),
      check_anime_type(false),
      validate_episode_number(false) {
}

bool Engine::Parse(std::wstring title, anime::Episode& episode) {
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
  episode.anime_id = Find(episode, match_options);

  if (give_score && !anime::IsValidId(episode.anime_id)) {
    ResetScores();
    ScoreTitle(episode);
  }

  return episode.anime_id;
}

bool Engine::Match(anime::Episode& episode, const anime::Item& anime_item,
                   const MatchOptions& match_options) {
  episode.anime_id = Find(episode, match_options);

  return episode.anime_id == anime_item.GetId();
}

////////////////////////////////////////////////////////////////////////////////

int Engine::Find(anime::Episode& episode, const MatchOptions& match_options) {
  InitializeNormalTitles();

  auto find_clean_title = [&](std::wstring title) {
    ToLower(title);
    auto it = normal_titles_.find(title);
    if (it != normal_titles_.end()) {
      episode.anime_id = *it->second.rbegin();  // TODO: Handle multiple IDs
      return true;
    }
    return false;
  };

  if (find_clean_title(episode.normal_title + episode.number)) {
    const anime::Item& anime_item = *AnimeDatabase.FindItem(episode.anime_id);
    if (anime_item.GetEpisodeCount() == 1) {
      episode.title += episode.number;
      episode.number.clear();
    } else {
      episode.anime_id = anime::ID_UNKNOWN;
    }
  }

  if (!anime::IsValidId(episode.anime_id)) {
    find_clean_title(episode.normal_title);
  }

  if (anime::IsValidId(episode.anime_id)) {
    const anime::Item& anime_item = *AnimeDatabase.FindItem(episode.anime_id);

    if (ValidateOptions(episode, anime_item, match_options)) {
      episode.anime_id = anime_item.GetId();

      // Assume episode 1, if matched a single-episode anime
      if (episode.number.empty() && anime_item.GetEpisodeCount() == 1)
        episode.number = L"1";
    }
  }

  return episode.anime_id;
}

bool Engine::ValidateOptions(anime::Episode& episode,
                             const anime::Item& anime_item,
                             const MatchOptions& match_options) {
  if (match_options.check_airing_date)
    if (!anime::IsAiredYet(anime_item))
      return false;

  if (match_options.check_anime_type) {
    // TODO: Check anime type, ignore if "ED", "OP", "PV"
  }

  if (match_options.validate_episode_number)
    if (!ValidateEpisodeNumber(episode, anime_item))
      return false;

  return true;
}

bool Engine::ValidateEpisodeNumber(anime::Episode& episode,
                                   const anime::Item& anime_item) {
  if (!anime_item.GetEpisodeCount())
    return true;

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

    return false;  // Episode number is out of range
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

void Engine::Normalize(std::wstring& title) {
  ToLower(title);
  EraseUnnecessary(title);
  // TODO: `2nd Season` = `Season 2` = `S2`, `II` -> `2`
  Transliterate(title);
  ErasePunctuation(title);
}

void Engine::InitializeNormalTitles() {
  if (normal_titles_.empty()) {
    for (const auto& it : AnimeDatabase.items) {
      UpdateNormalTitles(it.first);
    }
  }
}

void Engine::UpdateNormalTitles(int anime_id) {
  auto insert_title = [&](std::wstring title) {
    if (!title.empty()) {
      Normalize(title);
      normal_titles_[title].insert(anime_id);
    }
  };

  const auto& anime_item = *AnimeDatabase.FindItem(anime_id);

  insert_title(anime_item.GetTitle());
  insert_title(anime_item.GetEnglishTitle());

  for (const auto& synonym : anime_item.GetUserSynonyms())
    insert_title(synonym);
  for (const auto& synonym : anime_item.GetSynonyms())
    insert_title(synonym);
}

void Engine::ErasePunctuation(std::wstring& str) {
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
void Engine::EraseUnnecessary(std::wstring& str) {
  Replace(str, L" & ", L" and ", true);

  EraseLeft(str, L"the ");
  Replace(str, L" the ", L" ", true);

  Erase(str, L"episode ");
  Replace(str, L" specials", L" special", true);
}

void Engine::Transliterate(std::wstring& str) {
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

Engine::score_result_t Engine::GetScores() {
  score_result_t results;

  for (auto& score : scores_) {
    if (score.second == 0)
      continue;
    results.insert(std::pair<int, int>(score.second, score.first));
  }

  return results;
}

void Engine::ResetScores() {
  for (auto& score : scores_) {
    score.second = 0;
  }
}

void Engine::ScoreTitle(const anime::Episode& episode) {
  const std::wstring& title = episode.normal_title;

  for (auto& it : normal_titles_) {
    int anime_id = *it.second.begin();
    const auto& anime_item = *AnimeDatabase.FindItem(anime_id);
    const std::wstring& anime_title = it.first;

    const int score_bonus_small = 1;
    const int score_bonus_big = 5;
    const int score_min = std::abs(static_cast<int>(title.length()) -
                                   static_cast<int>(anime_title.length()));
    const int score_max = title.length() + anime_title.length();

    int score = score_max;

    score -= LevenshteinDistance(title, anime_title);

    score += LongestCommonSubsequenceLength(title, anime_title) * 2;
    score += LongestCommonSubstringLength(title, anime_title) * 4;

    if (score <= score_min)
      continue;

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
      scores_[anime_id] = score;
    }
  }
}

}  // namespace recognition
}  // namespace track