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

#include <set>

#include <anitomy/anitomy/keyword.h>

#include "track/recognition.h"

#include "base/log.h"
#include "base/string.h"
#include "media/anime.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "track/episode.h"
#include "track/episode_util.h"

namespace track::recognition {

bool Engine::ValidateOptions(anime::Episode& episode, int anime_id,
                             const MatchOptions& match_options,
                             bool redirect) const {
  auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return false;

  return ValidateOptions(episode, *anime_item, match_options, redirect);
}

bool Engine::ValidateOptions(anime::Episode& episode,
                             const anime::Item& anime_item,
                             const MatchOptions& match_options,
                             bool redirect) const {
  if (match_options.check_airing_date)
    if (anime_item.GetDateStart() && !anime::IsAiredYet(anime_item))
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
        LOGD(L"Redirection: {}:{} -> {}:{}",
             anime_item.GetId(), anime::GetEpisodeRange(episode),
             destination_id, anime::GetEpisodeRange(destination_range));
        episode.anime_id = destination_id;
        episode.set_episode_number_range(destination_range);
      }
      return true;  // Redirection available
    }
  }

  if (!anime::IsValidEpisodeCount(anime_item.GetEpisodeCount()))
    return true;  // Episode count is unknown, so anything goes

  return false;  // Episode number is out of range
}

////////////////////////////////////////////////////////////////////////////////

static bool ValidateAnitomyElement(std::wstring str,
                                   anitomy::ElementCategory category) {
  str = anitomy::keyword_manager.Normalize(str);
  anitomy::KeywordOptions options;

  bool found = anitomy::keyword_manager.Find(str, category, options);
  return found && options.valid;
}

bool Engine::IsBatchRelease(const anime::Episode& episode) const {
  const auto& elements = episode.elements();

  if (!elements.empty(anitomy::kElementVolumeNumber))
    return true;  // A volume is always a batch release

  if (!elements.empty(anitomy::kElementReleaseInformation)) {
    auto keywords = elements.get_all(anitomy::kElementReleaseInformation);
    for (const auto& keyword : keywords) {
      if (IsEqual(keyword, L"Batch"))
        return true;
    }
  }

  return false;
}

bool Engine::IsValidAnimeType(const anime::Episode& episode) const {
  const auto category = anitomy::kElementAnimeType;

  if (episode.elements().empty(category))
    return true;

  auto anime_types = episode.elements().get_all(category);
  for (const auto& anime_type : anime_types) {
    if (!ValidateAnitomyElement(anime_type, category)) {
      LOGD(episode.file_name_with_extension());
      return false;
    }
  }

  return true;
}

bool Engine::IsValidAnimeType(const std::wstring& path,
                              const ParseOptions& parse_options) const {
  anime::Episode episode;

  if (Parse(path, parse_options, episode))
    if (!IsValidAnimeType(episode))
      return false;

  return true;
}

bool Engine::IsValidAnimeType(const std::wstring& path) const {
  track::recognition::ParseOptions parse_options;
  parse_options.parse_path = true;
  parse_options.streaming_media = false;

  return IsValidAnimeType(path, parse_options);
}

bool Engine::IsValidFileExtension(const anime::Episode& episode) const {
  if (!IsValidFileExtension(episode.file_extension())) {
    LOGD(episode.file_name_with_extension());
    return false;
  }

  return true;
}

bool Engine::IsValidFileExtension(const std::wstring& extension) const {
  if (extension.empty() || extension.size() > 4)
    return false;

  return ValidateAnitomyElement(extension, anitomy::kElementFileExtension);
}

bool Engine::IsAudioFileExtension(const std::wstring& extension) const {
  static const std::set<std::wstring> extensions{
    L"aac", L"aiff", L"flac", L"m4a", L"mka", L"mp3", L"ogg", L"wav", L"wma",
  };
  return extensions.count(ToLower_Copy(extension));
}

}  // namespace track::recognition
