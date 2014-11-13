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

#include <anitomy/anitomy/anitomy.h>

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

RecognitionEngine::RecognitionEngine() {}

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

  // Retrieve file name from full path
  if (title.length() > 2 && title.at(1) == ':' && title.at(2) == '\\') {
    episode.folder = GetPathOnly(title);
    title = GetFileName(title);
  }

  // Ignore if the file is outside of root folders
  if (Settings.GetBool(taiga::kSync_Update_OutOfRoot))
    if (!episode.folder.empty() && !Settings.root_folders.empty())
      if (!anime::IsInsideRootFolders(episode.folder))
        return false;

  anitomy::Anitomy anitomy_instance;

  if (!anitomy_instance.Parse(title))
    return false;

  for (auto& element : anitomy_instance.elements()) {
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

  if (episode.title.empty())
    return false;

  episode.clean_title = episode.title;
  CleanTitle(episode.clean_title);

  return true;
}

////////////////////////////////////////////////////////////////////////////////

// Helper functions

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