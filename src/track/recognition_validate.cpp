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

#include <QString>
#include <algorithm>
#include <anitomy.hpp>
#include <ranges>

#include "media/anime.hpp"
#include "media/anime_db.hpp"
#include "track/episode.hpp"

namespace track::recognition {

namespace {

bool isValidEpisodeNumber(const Episode& episode, const anime::Details& item) {
  if (item.episode_count < 1) {
    return true;  // episode count is unknown, so anything goes
  }

  const auto numbers = episode.elements(anitomy::ElementKind::Episode);

  if (numbers.empty()) {
    if (item.episode_count == 1) {
      return true;  // single-episode anime can do without an episode number
    }

    const auto extension = episode.element(anitomy::ElementKind::FileExtension);
    if (extension.empty()) {
      return true;  // batch release
    }

    return false;  // no episode number to check against
  }

  // @TODO: This truncates decimal episode numbers (e.g. "07.5")
  const auto toInt = [](const std::string& value) { return QString::fromStdString(value).toInt(); };
  const int value = std::ranges::max(numbers | std::views::transform(toInt));

  if (value <= item.episode_count) return true;  // in range

  // @TODO: Attempt episode redirection via deps/anime-relations

  return false;  // out of range
}

}  // namespace

bool isValidMatch(const int id, const Episode& episode) {
  const auto item = anime::db.item(id);

  if (!item) return false;

  if (!isValidEpisodeNumber(episode, *item)) return false;

  return true;
}

}  // namespace track::recognition
