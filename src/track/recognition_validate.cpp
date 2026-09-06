/**
 * Taiga
 * Copyright (C) 2010-2026, Eren Okka
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

#include "recognition_validate.hpp"

#include <algorithm>
#include <anitomy.hpp>
#include <vector>

#include "base/string.hpp"
#include "media/anime.hpp"
#include "track/episode.hpp"

namespace track::recognition {

bool isValidEpisodeType(const Episode& episode) {
  const auto values = episode.elements(anitomy::ElementKind::Type);

  const auto isEpisodeTypeKeyword = [](const std::string& value) {
    // Matches Anitomy's keywords for `KeywordKind::EpisodeType`.
    // @TODO: Remove strings if Anitomy exposes `ElementKind::EpisodeType`.
    static const std::vector<std::string> keywords{
        "op", "opening", "ncop", "ed", "ending", "nced", "preview", "pv",
    };

    return std::ranges::any_of(keywords, [&value](const std::string& keyword) {
      return compareStrings(value, keyword, Qt::CaseInsensitive) == 0;
    });
  };

  return std::ranges::none_of(values, isEpisodeTypeKeyword);
}

bool isValidEpisodeNumber(const Episode& episode, const anime::Details& item) {
  if (item.episode_count < 1) {
    return true;  // episode count is unknown, so anything goes
  }

  const auto number = episode.getEpisodeNumber();

  if (!number) {
    if (item.episode_count == 1) {
      return true;  // single-episode anime can do without an episode number
    }

    const auto extension = episode.element(anitomy::ElementKind::FileExtension);
    if (extension.empty()) {
      return true;  // batch release
    }

    return false;  // no episode number to check against
  }

  return *number <= item.episode_count;
}

}  // namespace track::recognition
