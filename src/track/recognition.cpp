/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <anitomy/anitomy/anitomy.h>
#include <anitomy/anitomy/keyword.h>

#include "track/recognition.h"

#include "base/log.h"
#include "base/string.h"
#include "media/anime.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "taiga/settings.h"
#include "track/episode.h"

namespace track::recognition {

bool Engine::Parse(std::wstring filename, const ParseOptions& parse_options,
                   anime::Episode& episode) const {
  // Clear previous data
  episode.Clear();

  // Separate filename from path
  if (parse_options.parse_path && !parse_options.streaming_media) {
    if (filename.find_first_of(L"\\/") != filename.npos) {
      episode.folder = GetPathOnly(filename);
      filename = GetFileName(filename);
    }
  }

  if (filename.empty())
    return false;

  anitomy::Anitomy anitomy_instance;

  // Set Anitomy options
  if (parse_options.streaming_media)
    anitomy_instance.options().allowed_delimiters = L" ";
  Split(taiga::settings.GetRecognitionIgnoredStrings(), L"|",
        anitomy_instance.options().ignored_strings);

  if (!anitomy_instance.Parse(filename)) {
    LOGD(L"Could not parse filename: {}", filename);
    if (episode.folder.empty())  // If not, perhaps we can parse the path later on
      return false;
  }

  episode.set_elements(anitomy_instance.elements());

  ExtendAnimeTitle(episode);

  return true;
}

int Engine::Identify(anime::Episode& episode, bool give_score,
                     const MatchOptions& match_options) {
  std::set<int> anime_ids;

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
      LOGD(L"Merged title lookup succeeded: {}", episode.anime_title());
    }
  };

  if (!match_options.streaming_media) {
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
  }

  // Look up anime title
  if (anime_ids.empty()) {
    LookUpTitle(episode.anime_title(), anime_ids);
    valide_ids(episode);
  }

  // Look up parent directories
  if (anime_ids.empty() && !episode.folder.empty() &&
      taiga::settings.GetRecognitionLookupParentDirectories() &&
      episode.anime_type().empty()) {
    anime::Episode episode_from_directory(episode);
    episode_from_directory.elements().erase(anitomy::kElementAnimeTitle);
    if (GetTitleFromPath(episode_from_directory)) {
      LookUpTitle(episode_from_directory.anime_title(), anime_ids);
      valide_ids(episode_from_directory);
      if (!anime_ids.empty()) {
        std::swap(episode_from_directory, episode);
        LOGD(L"Parent directory lookup succeeded: {} -> {}",
             episode_from_directory.anime_title(),
             episode.anime_title());
      }
    }
  }

  // Figure out which ID is the one we're looking for
  if (anime::IsValidId(episode.anime_id)) {
    // We had a redirection while validating IDs
    if (!anime::db.Find(episode.anime_id, false)) {
      episode.anime_id = anime::ID_UNKNOWN;
      LOGD(L"Redirection failed, because destination ID is not available in the "
           L"database.");
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
      } else if (episode.elements().empty(anitomy::kElementVolumeNumber)) {
        auto anime_item = anime::db.Find(episode.anime_id);
        if (anime_item) {
          const int last_episode = [&anime_item]() {
            switch (anime_item->GetAiringStatus()) {
              case anime::SeriesStatus::FinishedAiring:
                return anime_item->GetEpisodeCount();
              case anime::SeriesStatus::Airing:
                return anime::GetLastEpisodeNumber(*anime_item);
              default:
                return 0;
            }
          }();
          if (last_episode)
            episode.set_episode_number_range({1, last_episode});
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
    for (const auto& it : anime::db.items) {
      UpdateTitles(it.second);
    }

    ReadRelations();
  }
}

void Engine::UpdateTitles(const anime::Item& anime_item, bool erase_ids) {
  const int anime_id = anime_item.GetId();

  db_[anime_id].normal_titles.clear();
  db_[anime_id].trigrams.clear();

  if (erase_ids) {
    auto erase_id = [&anime_id](Titles::container_t& titles) {
      for (auto& title : titles) {
        title.second.erase(anime_id);
      }
    };
    erase_id(titles_.alternative);
    erase_id(titles_.main);
    erase_id(titles_.user);
    erase_id(normal_titles_.alternative);
    erase_id(normal_titles_.main);
    erase_id(normal_titles_.user);
  }

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
  update_title(anime_item.GetJapaneseTitle(), titles_.main, normal_titles_.main);

  const auto& date = anime_item.GetDateStart();
  if (anime::IsValidDate(date)) {
    std::wstring year = ToWstr(date.year());
    if (anime_item.GetTitle().find(year) == std::wstring::npos) {
      update_title(anime_item.GetTitle() + L" (" + year + L")",
                   titles_.alternative, normal_titles_.alternative);
    }
  }

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

////////////////////////////////////////////////////////////////////////////////

void Engine::ExtendAnimeTitle(anime::Episode& episode) const {
  if (episode.anime_title().empty())
    return;

  // We need this because we don't have anime relation data. Otherwise we could
  // jump to a sequel after matching the first season of a series.
  const auto anime_season = episode.anime_season();
  if (anime_season > 1) {  // No need to append for the first season
    // The word "Season" is added for clarity; it's going to be erased in the
    // normalization process.
    episode.set_anime_title(episode.anime_title() +
                            L" Season " + ToWstr(anime_season));
  }

  // @TEMP: Handle cases such as "2nd Season Part 2" and "Second Season OVA".
  // Remove once the issue is handled by Anitomy. See issues #748 and #960 for
  // more information.
  if (anime_season > 1) {
    if (StartsWith(episode.episode_title(), L"Part ") ||
        StartsWith(episode.episode_title(), L"Cour ")) {
      episode.set_anime_title(episode.anime_title() + L" " +
                              episode.episode_title());
      episode.elements().erase(anitomy::kElementEpisodeTitle);
    }
    if (!episode.anime_type().empty() &&
        !EndsWith(episode.anime_title(), episode.anime_type())) {
      episode.set_anime_title(episode.anime_title() + L" " +
                              episode.anime_type());
    }
  }

  // Some anime share the same title, and they're differentiated by their airing
  // date. We also come across year values in batch releases, where they appear
  // as additional metadata.
  // As it turns out, these numbers aren't always reliable as a means to discard
  // anime entries. So we make do with appending the value to the title, which
  // handles the first use case at the expense of complicating the second one.
  const auto anime_year = episode.anime_year();
  if (anime_year > 0) {
    episode.set_anime_title(episode.anime_title() +
                            L" (" + ToWstr(anime_year) + L")");
  }

  // @TEMP: Remove once we're able to redirect fractional episode numbers.
  if (!episode.elements().empty(anitomy::kElementEpisodeNumber)) {
    const auto episode_number = episode.elements().get(anitomy::kElementEpisodeNumber);
    if (episode_number.find(L'.') != episode_number.npos) {
      episode.set_anime_title(episode.anime_title() +
                              L" Episode " + episode_number);
      episode.elements().erase(anitomy::kElementEpisodeNumber);
    }
  }
}

bool Engine::GetTitleFromPath(anime::Episode& episode) {
  if (episode.folder.empty())
    return false;

  std::wstring path = episode.folder;

  for (const auto& library_folder : taiga::settings.GetLibraryFolders()) {
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
      if (anitomy_instance.elements().empty(anitomy::kElementAnimeTitle))
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

  if (episode.anime_title().empty()) {
    return false;
  } else {
    // We're parsing the directory name in case it looks like
    // "[Fansub] Anime Title [Stuff]" rather than just "Anime Title".
    anitomy::Anitomy anitomy_instance;
    anitomy_instance.options().parse_episode_number = false;
    anitomy_instance.options().parse_episode_title = false;
    anitomy_instance.options().parse_file_extension = false;
    anitomy_instance.options().parse_release_group = true;
    if (anitomy_instance.Parse(episode.anime_title())) {
      auto& elements = anitomy_instance.elements();
      const auto valid_elements = {
        anitomy::kElementAnimeTitle,
        anitomy::kElementAnimeYear
      };
      for (const auto& element : valid_elements) {
        if (!elements.empty(element))
          episode.SetElementValue(element, elements.get(element));
      }
    }
  }

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

  ExtendAnimeTitle(episode);

  return true;
}

}  // namespace track::recognition
