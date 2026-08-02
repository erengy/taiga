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

#include "recognition.hpp"

#include <QFileInfo>
#include <algorithm>
#include <anitomy.hpp>
#include <ranges>
#include <vector>

#include "media/anime.hpp"
#include "track/episode.hpp"
#include "track/recognition_cache.hpp"
#include "track/recognition_normalize.hpp"
#include "track/recognition_path.hpp"
#include "track/recognition_validate.hpp"

namespace track::recognition {

Episode parse(std::string_view input, const anitomy::Options options) {
  Episode episode;

  auto elements = anitomy::parse(input, options);
  episode.setElements(elements);

  return episode;
}

Episode parseFileInfo(const QFileInfo& info, const anitomy::Options options) {
  const auto fileName = info.fileName().toStdString();

  Episode episode = track::recognition::parse(fileName, options);

  if (!episode.contains(anitomy::ElementKind::Title)) {
    const auto title = findTitleFromPath(info);
    if (!title.empty()) {
      episode.addElement(anitomy::ElementKind::Title, title);
    }
  }

  return episode;
}

int identify(Episode& episode) {
  cache()->init();

  const auto title = episode.element(anitomy::ElementKind::Title);
  const auto normalizedTitle = normalize(title);

  std::vector<Cache::Data::Match> matches;

  if (const auto data = cache()->find(normalizedTitle)) {
    matches.append_range(data->matches | std::views::values | std::ranges::to<std::vector>());
  }

  std::ranges::sort(matches, std::ranges::greater{}, &Cache::Data::Match::weight);

  for (const auto& match : matches) {
    if (isValidMatch(match.id, episode)) return match.id;
  }

  return anime::kUnknownId;
}

bool isVideoFile(const Episode& episode) {
  // This relies on Anitomy tagging `FileExtension` only for video
  // container extensions.
  return episode.contains(anitomy::ElementKind::FileExtension);
}

}  // namespace track::recognition
