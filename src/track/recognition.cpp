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
#include <cmath>
#include <set>

#include <anitomy/anitomy/anitomy.h>
#include <anitomy/anitomy/keyword.h>
#include <utf8proc/utf8proc.h>

#include "base/log.h"
#include "base/string.h"
#include "library/anime.h"
#include "library/anime_db.h"
#include "library/anime_episode.h"
#include "library/anime_util.h"
#include "taiga/settings.h"
#include "track/recognition.h"

static enum NormalizationType {
  kNormalizeMinimal,
  kNormalizeForTrigrams,
  kNormalizeFull,
};

track::recognition::Engine Meow;

namespace track {
namespace recognition {

bool Engine::Parse(std::wstring title, anime::Episode& episode) const {
  episode.Clear();  // Clear previous data

  if (title.empty())
    return false;

  anitomy::Anitomy anitomy_instance;

  if (episode.streaming_media) {
    anitomy_instance.options().allowed_delimiters = L" ";
  } else {
    if (title.find_first_of(L"\\/") != title.npos) {
      episode.folder = GetPathOnly(title);
      title = GetFileName(title);
    }
  }

  const auto& ignored_strings = Settings[taiga::kRecognition_IgnoredStrings];
  if (!ignored_strings.empty())
    Split(ignored_strings, L"|", anitomy_instance.options().ignored_strings);

  if (!anitomy_instance.Parse(title)) {
    LOG(LevelDebug, L"Could not parse filename: " + title);
    if (episode.folder.empty())  // If not, perhaps we can parse the path later on
      return false;
  }

  episode.set_elements(anitomy_instance.elements());

  // Append season number and year to title
  auto anime_season = episode.anime_season();
  if (anime_season > 1 && !episode.anime_title().empty())
    episode.set_anime_title(episode.anime_title() + L" " +
                            ToWstr(anime_season));
  auto anime_year = episode.anime_year();
  if (anime_year > 0 && !episode.anime_title().empty())
    episode.set_anime_title(episode.anime_title() + L" (" +
                            ToWstr(anime_year) + L")");

  return true;
}

int Engine::Identify(anime::Episode& episode, bool give_score,
                     const MatchOptions& match_options) {
  std::set<int> anime_ids;

  if (give_score)
    scores_.clear();

  auto valide_ids = [&](anime::Episode& episode) {
    for (auto it = anime_ids.begin(); it != anime_ids.end(); ) {
      if (!ValidateOptions(episode, *it, match_options, true)) {
        it = anime_ids.erase(it);
      } else {
        ++it;
      }
    }
  };

  InitializeTitles();

  // Look up the title in our database
  LookUpTitle(episode.anime_title(), anime_ids);
  valide_ids(episode);

  // Look up parent directories
  if (anime_ids.empty() && !episode.folder.empty()) {
    anime::Episode episode_from_directory(episode);
    episode_from_directory.elements().erase(anitomy::kElementAnimeTitle);
    if (GetTitleFromPath(episode_from_directory)) {
      LookUpTitle(episode_from_directory.anime_title(), anime_ids);
      valide_ids(episode_from_directory);
      if (!anime_ids.empty()) {
        std::swap(episode_from_directory, episode);
        LOG(LevelDebug, L"Parent directory lookup succeeded: " +
                        episode.anime_title());
      }
    }
  }

  // Figure out which ID is the one we're looking for
  if (anime::IsValidId(episode.anime_id)) {
    // We had a redirection while validating IDs
  } else if (anime_ids.size() == 1) {
    episode.anime_id = *anime_ids.begin();
  } else if (anime_ids.size() > 1) {
    episode.anime_id = ScoreTitle(episode, anime_ids, match_options);
  } else if (anime_ids.empty() && give_score) {
    ScoreTitle(episode, anime_ids, match_options);
  }

  // Post-processing
  if (anime::IsValidId(episode.anime_id)) {
    // Here we check the element rather than episode_number(), in order to
    // prevent overwriting episode 0.
    if (episode.elements().empty(anitomy::kElementEpisodeNumber)) {
      if (!episode.file_extension().empty()) {
        episode.set_episode_number(1);
      } else {
        const auto& anime_item = *AnimeDatabase.FindItem(episode.anime_id);
        int episode_count = anime_item.GetEpisodeCount();
        episode.set_episode_number_range(std::make_pair(1, episode_count));
      }
    }
  }

  return episode.anime_id;
}

bool Engine::Search(const std::wstring& title, std::vector<int>& anime_ids) {
  anime::Episode episode;
  episode.set_anime_title(title);

  std::set<int> empty_set;
  track::recognition::MatchOptions default_options;

  InitializeTitles();
  scores_.clear();

  ScoreTitle(episode, empty_set, default_options);

  for (const auto& score : scores_) {
    anime_ids.push_back(score.first);
  }

  return !anime_ids.empty();
}

////////////////////////////////////////////////////////////////////////////////

bool Engine::ValidateOptions(anime::Episode& episode, int anime_id,
                             const MatchOptions& match_options,
                             bool redirect) const {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (!anime_item)
    return false;

  return ValidateOptions(episode, *anime_item, match_options, redirect);
}

bool Engine::ValidateOptions(anime::Episode& episode,
                             const anime::Item& anime_item,
                             const MatchOptions& match_options,
                             bool redirect) const {
  if (match_options.check_airing_date)
    if (!anime::IsAiredYet(anime_item))
      return false;

  if (match_options.check_anime_type)
    if (!IsValidAnimeType(episode))
      return false;

  if (match_options.check_episode_number)
    if (!ValidateEpisodeNumber(episode, anime_item, match_options, redirect))
      return false;

  return true;
}

bool Engine::ValidateEpisodeNumber(anime::Episode& episode,
                                   const anime::Item& anime_item,
                                   const MatchOptions& match_options,
                                   bool redirect) const {
  if (episode.elements().empty(anitomy::kElementEpisodeNumber)) {
    if (anime_item.GetEpisodeCount() == 1)
      return true;  // Single-episode anime can do without an episode number
    if (episode.file_extension().empty())
      return true;  // It's a batch release
  }

  auto range = episode.episode_number_range();

  if (range.second > 0 &&  // We need this to be able to redirect episode 0
      range.second <= anime_item.GetEpisodeCount()) {
    return true;  // Episode number is within range
  }

  if (match_options.allow_sequels) {
    int destination_id = anime::ID_UNKNOWN;
    std::pair<int, int> destination_range;
    if (SearchEpisodeRedirection(anime_item.GetId(), range,
                                 destination_id, destination_range)) {
      if (redirect) {
        std::wstring text = L"Redirection: " + ToWstr(anime_item.GetId()) +
                            L":" + anime::GetEpisodeRange(episode) + L" -> ";
        episode.anime_id = destination_id;
        episode.set_episode_number_range(destination_range);
        text += ToWstr(destination_id) + L":" + anime::GetEpisodeRange(episode);
        LOG(LevelDebug, text);
      }
      return true;  // Redirection available
    }
  }

  if (!anime::IsValidEpisodeCount(anime_item.GetEpisodeCount()))
    return true;  // Episode count is unknown, so anything goes

  return false;  // Episode number is out of range
}

////////////////////////////////////////////////////////////////////////////////

void Engine::InitializeTitles() {
  static bool initialized = false;

  if (!initialized) {
    initialized = true;
    for (const auto& it : AnimeDatabase.items) {
      UpdateTitles(it.second);
    }

    ReadRelations();
  }
}

void Engine::UpdateTitles(const anime::Item& anime_item) {
  int anime_id = anime_item.GetId();

  db_[anime_id].normal_titles.clear();
  db_[anime_id].trigrams.clear();

  auto update_title = [&](std::wstring title,
                          Titles::container_t& titles,
                          Titles::container_t& normal_titles) {
    if (!title.empty()) {
      Normalize(title, kNormalizeMinimal, false);
      titles[title].insert(anime_id);

      Normalize(title, kNormalizeForTrigrams, true);
      trigram_container_t trigrams;
      GetTrigrams(title, trigrams);
      db_[anime_id].trigrams.push_back(trigrams);
      db_[anime_id].normal_titles.push_back(title);

      Normalize(title, kNormalizeFull, true);
      normal_titles[title].insert(anime_id);
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

int Engine::LookUpTitle(std::wstring title, std::set<int>& anime_ids) const {
  int anime_id = anime::ID_UNKNOWN;

  auto find_title = [&](const std::wstring& title,
                        const Titles::container_t& container) {
    if (!anime::IsValidId(anime_id)) {
      auto it = container.find(title.c_str());
      if (it != container.end()) {
        anime_ids.insert(it->second.begin(), it->second.end());
        if (anime_ids.size() == 1)
          anime_id = *anime_ids.begin();
      }
    }
  };

  Normalize(title, kNormalizeMinimal, false);
  find_title(title, titles_.user);
  find_title(title, titles_.main);
  find_title(title, titles_.alternative);

  if (anime_ids.size() == 1)
    return anime_id;

  Normalize(title, kNormalizeFull, true);
  find_title(title, normal_titles_.user);
  find_title(title, normal_titles_.main);
  find_title(title, normal_titles_.alternative);

  return anime_id;
}

bool Engine::GetTitleFromPath(anime::Episode& episode) {
  if (episode.folder.empty())
    return false;

  std::wstring path = episode.folder;

  for (const auto& library_folder : Settings.library_folders) {
    if (StartsWith(path, library_folder)) {
      path.erase(0, library_folder.size());
      break;
    }
  }

  Trim(path, L"\\/");
  std::vector<std::wstring> directories;
  Tokenize(path, L"\\/", directories);

  auto is_invalid_string = [](std::wstring str) {
    if (str.find(L':') != str.npos)  // drive letter
      return true;
    static std::set<std::wstring> invalid_strings{
      L"anime", L"download", L"downloads", L"extra", L"extras",
    };
    ToLower(str);
    return invalid_strings.count(str) > 0;
  };

  auto get_season_number = [](const std::wstring& str) {
    anitomy::Anitomy anitomy_instance;
    anitomy_instance.options().parse_episode_number = false;
    anitomy_instance.options().parse_episode_title = false;
    anitomy_instance.options().parse_file_extension = false;
    anitomy_instance.options().parse_release_group = false;
    anitomy_instance.Parse(str);
    auto it = anitomy_instance.elements().find(anitomy::kElementAnimeSeason);
    if (it != anitomy_instance.elements().end())
      return ToInt(it->second);
    return 0;
  };

  int current_depth = 0;
  const int max_allowed_depth = 1;

  for (auto it = directories.rbegin(); it != directories.rend(); ++it) {
    if (current_depth++ > max_allowed_depth)
      break;
    const auto& directory = *it;
    if (directory.empty() || is_invalid_string(directory))
      break;
    int number = get_season_number(directory);
    if (number) {
      if (!episode.anime_season())
        episode.set_anime_season(number);
    } else {
      episode.set_anime_title(directory);
      break;
    }
  }

  if (episode.anime_title().empty())
    return false;

  auto find_number_in_string = [](const std::wstring& str) {
    auto it = std::find_if(str.begin(), str.end(), IsNumericChar);
    return it == str.end() ? str.npos : (it - str.begin());
  };

  if (episode.elements().empty(anitomy::kElementEpisodeNumber)) {
    const auto& filename = episode.file_name();
    auto pos = find_number_in_string(filename);
    if (pos == 0)  // begins with a number (e.g. "01.mkv", "02 - Title.mkv")
      episode.set_episode_number(ToInt(filename.substr(pos)));
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

void Engine::Normalize(std::wstring& title, int type,
                       bool normalized_before) const {
  if (!normalized_before) {
    ConvertRomanNumbers(title);
    Transliterate(title);
    NormalizeUnicode(title);  // Title is lower case after this point, due to UTF8PROC_CASEFOLD
    ConvertOrdinalNumbers(title);
    ConvertSeasonNumbers(title);
    EraseUnnecessary(title);
  }

  switch (type) {
    case kNormalizeMinimal:
      Trim(title);
      while (ReplaceString(title, 0, L"  ", L" ", false, true));
      break;
    case kNormalizeForTrigrams:
      ErasePunctuation(title, true);
      while (ReplaceString(title, 0, L"  ", L" ", false, true));
      break;
    default:
    case kNormalizeFull:
      ErasePunctuation(title);
      break;
  }
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
  ReplaceString(str, 0, L"(tv)", L"", true, true);
}

void Engine::ErasePunctuation(std::wstring& str, bool for_trigrams) const {
  bool is_trailing = for_trigrams;

  auto is_removable = [&](const wchar_t c) {
    // Control codes, white-space and punctuation characters
    if (c <= 0xFF && !IsAlphanumericChar(c))
      if (!for_trigrams ||
          (c != L' ' && (!is_trailing || (c != L')' && c != L']'))))
        return true;
    // Unicode stars, hearts, notes, etc.
    if (c > 0x2000 && c < 0x2767)
      return true;
    // Valid character
    return false;
  };

  auto it_end = !for_trigrams ? str.end() :
      std::find_if_not(str.rbegin(), str.rend(), is_removable).base();
  is_trailing = false;

  auto it = std::remove_if(str.begin(), it_end, is_removable);

  if (for_trigrams)
    it = std::copy(it_end, str.end(), it);

  if (it != str.end())
    str.erase(it, str.end());
}

////////////////////////////////////////////////////////////////////////////////

sorted_scores_t Engine::GetScores() const {
  return scores_;
}

int Engine::ScoreTitle(anime::Episode& episode, const std::set<int>& anime_ids,
                       const MatchOptions& match_options) {
  scores_t trigram_results;

  auto normal_title = episode.anime_title();
  Normalize(normal_title, kNormalizeForTrigrams, false);

  trigram_container_t t1;
  GetTrigrams(normal_title, t1);

  auto calculate_trigram_results = [&](int anime_id) {
    for (const auto& t2 : db_[anime_id].trigrams) {
      double result = CompareTrigrams(t1, t2);
      if (result > 0.1) {
        auto& target = trigram_results[anime_id];
        target = max(target, result);
      }
    }
  };

  if (!anime_ids.empty()) {
    for (const auto& id : anime_ids) {
      calculate_trigram_results(id);
    }
  } else {
    for (const auto& it : AnimeDatabase.items) {
      if (ValidateOptions(episode, it.second, match_options, false))
        calculate_trigram_results(it.first);
    }
  }

  return ScoreTitle(normal_title, episode, trigram_results);
}

static double CustomScore(const std::wstring& title, const std::wstring& str) {
  double length_min = min(title.size(), str.size());
  double length_max = max(title.size(), str.size());
  double length_ratio = length_min / length_max;

  double score = 0.0;

  if (StartsWith(title, str) || StartsWith(str, title)) {
    score = length_ratio;
  } else if (InStr(title, str) > -1 || InStr(str, title) > -1) {
    score = length_ratio * 0.9;
  } else {
    auto length_lcs = LongestCommonSubsequenceLength(title, str);
    auto lcs_score = length_lcs / length_max;
    score = lcs_score * 0.8;

    auto mismatch = std::mismatch(title.begin(), title.end(), str.begin());
    auto distance = std::distance(title.begin(), mismatch.first);
    if (distance > 0) {
      auto distance_score = distance / length_min;
      score = max(score, distance_score * 0.7);
    }
  }

  return score;
};

static double BonusScore(const anime::Episode& episode, int id) {
  double score = 0.0;
  auto anime_item = AnimeDatabase.FindItem(id);

  auto anime_year = episode.anime_year();
  if (anime_year)
    if (anime_year == anime_item->GetDateStart().year)
      score += 0.1;

  auto anime_type = anime::TranslateType(episode.anime_type());
  if (anime_type != anime::kUnknownType)
    if (anime_type == anime_item->GetType())
      score += 0.1;

  return score;
};

int Engine::ScoreTitle(const std::wstring& str, const anime::Episode& episode,
                       const scores_t& trigram_results) {
  scores_t jaro_winkler, levenshtein, custom, bonus;

  for (const auto& trigram_result : trigram_results) {
    int id = trigram_result.first;

    // Calculate individual scores for all titles
    for (auto& title : db_[id].normal_titles) {
      jaro_winkler[id] = max(jaro_winkler[id], JaroWinklerDistance(title, str));
      levenshtein[id] = max(levenshtein[id], LevenshteinDistance(title, str));
      custom[id] = max(custom[id], CustomScore(title, str));
    }
    bonus[id] = BonusScore(episode, id);

    // Calculate the average score for the ID
    double score =
        (((1.0 * jaro_winkler[id]) +
          (0.5 * std::pow(custom[id], 0.66)) +
          (0.3 * std::pow(levenshtein[id], 0.8)) +
          (0.2 * std::pow(trigram_result.second, 0.8))) / 2.0) + bonus[id];
    if (score >= 0.3)
      scores_.push_back(std::make_pair(id, score));
  }

  // Sort scores in descending order, then limit the results
  std::stable_sort(scores_.begin(), scores_.end(),
      [&](const std::pair<int, double>& a,
          const std::pair<int, double>& b) {
        return a.second > b.second;
      });
  if (scores_.size() > 20)
    scores_.resize(20);

  double score_1st = scores_.size() > 0 ? scores_.at(0).second : 0.0;
  double score_2nd = scores_.size() > 1 ? scores_.at(1).second : 0.0;

  if (score_1st >= 1.0 && score_1st != score_2nd)
    return scores_.front().first;

  return anime::ID_UNKNOWN;
}

////////////////////////////////////////////////////////////////////////////////

static bool ValidateAnitomyElement(std::wstring str,
                                   anitomy::ElementCategory category) {
  str = anitomy::keyword_manager.Normalize(str);
  anitomy::KeywordOptions options;

  bool found = anitomy::keyword_manager.Find(str, category, options);
  return found && options.valid;
}

bool Engine::IsValidAnimeType(const anime::Episode& episode) const {
  auto anime_type = episode.anime_type();

  if (anime_type.empty())
    return true;

  if (!ValidateAnitomyElement(anime_type, anitomy::kElementAnimeType)) {
    LOG(LevelDebug, episode.file_name_with_extension());
    return false;
  }

  return true;
}

bool Engine::IsValidFileExtension(const anime::Episode& episode) const {
  if (!IsValidFileExtension(episode.file_extension())) {
    LOG(LevelDebug, episode.file_name_with_extension());
    return false;
  }

  return true;
}

bool Engine::IsValidFileExtension(const std::wstring& extension) const {
  if (extension.empty() || extension.size() > 4)
    return false;

  return ValidateAnitomyElement(extension, anitomy::kElementFileExtension);
}

}  // namespace recognition
}  // namespace track