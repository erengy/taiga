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
#include <anitomy/anitomy/keyword.h>

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

bool Engine::Parse(std::wstring title, anime::Episode& episode,
                   const ParseOptions& parse_options) const {
  episode.Clear();  // Clear previous data

  if (title.empty())
    return false;

  anitomy::Anitomy anitomy_instance;

  if (parse_options.streaming_media) {
    anitomy_instance.options().allowed_delimiters = L" ";
  } else if (parse_options.parse_path) {
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

  // Append season number to title
  const auto anime_season = episode.anime_season();
  if (anime_season > 1 && !episode.anime_title().empty())
    episode.set_anime_title(
        episode.anime_title() + L" Season " + ToWstr(anime_season));
  // Append year to title
  const auto anime_year = episode.anime_year();
  if (anime_year > 0 && !episode.anime_title().empty())
    episode.set_anime_title(
        episode.anime_title() + L" (" + ToWstr(anime_year) + L")");
  // TEMP: Append episode number to title
  // We're going to get rid of this once we're able to redirect fractional
  // episode numbers.
  if (!episode.elements().empty(anitomy::kElementEpisodeNumber)) {
    const auto& episode_number = episode.elements().get(anitomy::kElementEpisodeNumber);
    if (episode_number.find(L'.') != episode_number.npos) {
      episode.set_anime_title(
          episode.anime_title() + L" Episode " + episode_number);
      episode.elements().erase(anitomy::kElementEpisodeNumber);
    }
  }

  return true;
}

int Engine::Identify(anime::Episode& episode, bool give_score,
                     const MatchOptions& match_options) {
  std::set<int> anime_ids;

  if (give_score)
    scores_.clear();

  InitializeTitles();

  auto valide_ids = [&](anime::Episode& episode) {
    for (auto it = anime_ids.begin(); it != anime_ids.end(); ) {
      if (!ValidateOptions(episode, *it, match_options, true)) {
        it = anime_ids.erase(it);
      } else {
        ++it;
      }
    }
  };

  auto look_up_merged_title = [&](
      const std::initializer_list<anitomy::ElementCategory>& elements) {
    anime::Episode episode_merged_title(episode);
    auto merged_title = episode.anime_title();
    for (const auto& element : elements) {
      merged_title += L" " + episode.elements().get(element);
      episode_merged_title.elements().erase(element);
    }
    episode_merged_title.set_anime_title(merged_title);
    LookUpTitle(episode_merged_title.anime_title(), anime_ids);
    valide_ids(episode_merged_title);
    if (!anime_ids.empty()) {
      std::swap(episode_merged_title, episode);
      LOG(LevelDebug, L"Merged title lookup succeeded: " +
                      episode.anime_title());
    }
  };

  // Look up anime title + episode number + episode title
  if (!episode.elements().empty(anitomy::kElementEpisodeNumber) &&
      !episode.elements().empty(anitomy::kElementEpisodeTitle)) {
    static const auto elements =
        {anitomy::kElementEpisodeNumber, anitomy::kElementEpisodeTitle};
    look_up_merged_title(elements);
  }
  // Look up anime title + episode number
  if (!episode.elements().empty(anitomy::kElementEpisodeNumber) &&
      (episode.elements().empty(anitomy::kElementFileExtension) ||
       !episode.elements().empty(anitomy::kElementAnimeType))) {
    static const auto elements = {anitomy::kElementEpisodeNumber};
    look_up_merged_title(elements);
  }

  // Look up anime title
  if (anime_ids.empty()) {
    LookUpTitle(episode.anime_title(), anime_ids);
    valide_ids(episode);
  }

  // Look up parent directories
  if (anime_ids.empty() && !episode.folder.empty() &&
      Settings.GetBool(taiga::kRecognition_LookupParentDirectories) &&
      episode.anime_type().empty()) {
    anime::Episode episode_from_directory(episode);
    episode_from_directory.elements().erase(anitomy::kElementAnimeTitle);
    if (GetTitleFromPath(episode_from_directory)) {
      LookUpTitle(episode_from_directory.anime_title(), anime_ids);
      valide_ids(episode_from_directory);
      if (!anime_ids.empty()) {
        std::swap(episode_from_directory, episode);
        LOG(LevelDebug, L"Parent directory lookup succeeded: " +
                        episode_from_directory.anime_title() + L" -> " +
                        episode.anime_title());
      }
    }
  }

  // Figure out which ID is the one we're looking for
  if (anime::IsValidId(episode.anime_id)) {
    // We had a redirection while validating IDs
    if (!AnimeDatabase.FindItem(episode.anime_id, false)) {
      episode.anime_id = anime::ID_UNKNOWN;
      LOG(LevelDebug, L"Redirection failed, because destination ID is not "
                      L"available in the database.");
    }
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
        auto anime_item = AnimeDatabase.FindItem(episode.anime_id);
        if (anime_item) {
          int episode_count = anime_item->GetEpisodeCount();
          episode.set_episode_number_range(std::make_pair(1, episode_count));
        }
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
      Normalize(title, kNormalizeForTrigrams, false);
      trigram_container_t trigrams;
      GetTrigrams(title, trigrams);
      db_[anime_id].trigrams.push_back(trigrams);
      db_[anime_id].normal_titles.push_back(title);

      Normalize(title, kNormalizeForLookup, true);
      titles[title].insert(anime_id);

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

  Normalize(title, kNormalizeForLookup, false);
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
      L"ANIME", L"DOWNLOAD", L"DOWNLOADS", L"EXTRA", L"EXTRAS",
    };
    ToUpper(str);
    if (invalid_strings.count(str) > 0)
      return true;
    anitomy::keyword_manager.Normalize(str);
    anitomy::ElementCategory category = anitomy::kElementUnknown;
    anitomy::KeywordOptions options;
    return anitomy::keyword_manager.Find(str, category, options);
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

}  // namespace recognition
}  // namespace track