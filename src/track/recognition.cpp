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

  episode.clean_title = episode.title;
  CleanTitle(episode.clean_title);

  if (episode.clean_title.empty()) {
    LOG(LevelWarning, L"episode.clean_title is empty for file: " + episode.file);
  }

  return !episode.clean_title.empty();
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
  InitializeCleanTitles();

  auto find_clean_title = [&](std::wstring title) {
    ToLower(title);
    auto it = clean_titles_.find(title);
    if (it != clean_titles_.end()) {
      episode.anime_id = *it->second.begin();  // TODO: Handle multiple IDs
      return true;
    }
    return false;
  };

  if (find_clean_title(episode.clean_title + episode.number)) {
    const anime::Item& anime_item = *AnimeDatabase.FindItem(episode.anime_id);
    if (anime_item.GetEpisodeCount() == 1) {
      episode.title += episode.number;
      episode.number.clear();
    } else {
      episode.anime_id = anime::ID_UNKNOWN;
    }
  }

  if (!anime::IsValidId(episode.anime_id)) {
    find_clean_title(episode.clean_title);
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

void Engine::CleanTitle(std::wstring& title) {
  if (title.empty())
    return;

  EraseUnnecessary(title);
  TransliterateSpecial(title);
  ErasePunctuation(title);

  ToLower(title);

  // TODO: `2nd Season` = `Season 2` = `S2`, `II` = `2`
}

// TODO: Too damn slow.
void Engine::InitializeCleanTitles() {
  if (!clean_titles_.empty())
    return;

  for (const auto& it : AnimeDatabase.items) {
    //if (it.second.IsInList())
      UpdateCleanTitles(it.first);
  }
}

void Engine::UpdateCleanTitles(int anime_id) {
  for (auto& clean_title : clean_titles_) {
    clean_title.second.erase(anime_id);
  }

  auto insert_title = [&](std::wstring title) {
    CleanTitle(title);
    clean_titles_[title].insert(anime_id);
  };

  auto anime_item = AnimeDatabase.FindItem(anime_id);

  // Main title
  insert_title(anime_item->GetTitle());

  // English title
  if (!anime_item->GetEnglishTitle().empty())
    insert_title(anime_item->GetEnglishTitle());

  // Synonyms
  if (!anime_item->GetUserSynonyms().empty()) {
    for (const auto& synonym : anime_item->GetUserSynonyms()) {
      insert_title(synonym);
    }
  }
  if (!anime_item->GetSynonyms().empty()) {
    for (const auto& synonym : anime_item->GetSynonyms()) {
      insert_title(synonym);
    }
  }
}

void Engine::ErasePunctuation(std::wstring& str) {
  auto rlast = std::find_if(str.rbegin(), str.rend(),
      [](wchar_t c) -> bool {
        return !(c == L'!' ||  // "Hayate no Gotoku!", "K-ON!"...
                 c == L'+' ||  // "Needless+"
                 c == L'\'');  // "Gintama'"
      });

  auto it = std::remove_if(str.begin(), rlast.base(),
      [](int c) -> bool {
        // Control codes, white-space and punctuation characters
        if (c <= 255 && !isalnum(c))
          return true;
        // Unicode stars, hearts, notes, etc. (0x2000-0x2767)
        if (c > 8192 && c < 10087)
          return true;
        // Valid character
        return false;
      });

  std::copy(rlast.base(), str.end(), it);

  str.resize(str.size() - (rlast.base() - it));
}

void Engine::EraseUnnecessary(std::wstring& str) {
  EraseLeft(str, L"the ", true);
  Replace(str, L" the ", L" ", false, true);
  Erase(str, L"episode ", true);
  Erase(str, L" ep.", true);
  Replace(str, L" specials", L" special", false, true);
}

// TODO: make faster
void Engine::TransliterateSpecial(std::wstring& str) {
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
  const std::wstring& title = episode.clean_title;

  for (auto& it : clean_titles_) {
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