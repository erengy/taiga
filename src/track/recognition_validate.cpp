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
#include <utility>
#include <vector>

#include "base/string.hpp"
#include "media/anime.hpp"
#include "media/anime_db.hpp"
#include "track/episode.hpp"

namespace track::recognition {

namespace {

anime::Type toAnimeType(const std::string& value) {
  // This table is supposed to match Anitomy's keywords for `KeywordKind::Type`.
  static const std::vector<std::pair<std::string, anime::Type>> types{
      {"tv", anime::Type::Tv},
      {"movie", anime::Type::Movie},
      {"gekijouban", anime::Type::Movie},
      {"oad", anime::Type::Ova},
      {"oav", anime::Type::Ova},
      {"ova", anime::Type::Ova},
      {"ona", anime::Type::Ona},
      {"sp", anime::Type::Special},
      {"special", anime::Type::Special},
      {"specials", anime::Type::Special},
  };

  const auto it = std::ranges::find_if(types, [&value](const auto& type) {
    return compareStrings(value, type.first, Qt::CaseInsensitive) == 0;
  });

  return it != types.end() ? it->second : anime::Type::Unknown;
}

bool isValidAnimeType(const Episode& episode, const anime::Details& item) {
  if (item.type == anime::Type::Unknown) {
    return true;  // anime's type is unknown
  }

  const auto value = episode.element(anitomy::ElementKind::Type);
  if (value.empty()) {
    return true;  // nothing parsed to validate
  }

  const auto type = toAnimeType(value);
  if (type == anime::Type::Unknown) {
    return true;  // not a recognized type keyword
  }

  return type == item.type;
}

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

  if (!isValidAnimeType(episode, *item)) return false;

  if (!isValidEpisodeNumber(episode, *item)) return false;

  return true;
}

}  // namespace track::recognition
