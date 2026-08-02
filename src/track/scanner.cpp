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

#include "scanner.hpp"

#include <QDirIterator>
#include <algorithm>
#include <optional>
#include <ranges>

#include "track/episode.hpp"
#include "track/recognition.hpp"

namespace track {

static bool containsEpisodeNumber(const Episode& episode, const int episode_number) {
  const auto numbers = episode.elements(anitomy::ElementKind::Episode);
  if (numbers.empty()) return false;

  const auto toInt = [](const std::string& value) { return QString::fromStdString(value).toInt(); };
  const auto [low, high] = std::ranges::minmax(numbers | std::views::transform(toInt));

  return low <= episode_number && episode_number <= high;
}

std::optional<QString> findEpisode(const QString& path, const int anime_id,
                                   const int episode_number) {
  QDirIterator it{path, QDir::Files, QDirIterator::Subdirectories};

  while (it.hasNext()) {
    const auto info = it.nextFileInfo();

    if (!info.isFile()) continue;

    auto episode = recognition::parseFileInfo(info);

    if (!recognition::isVideoFile(episode)) continue;

    if (!containsEpisodeNumber(episode, episode_number)) continue;

    if (track::recognition::identify(episode) != anime_id) continue;

    return info.filePath();
  }

  return std::nullopt;
}

std::optional<QString> findFolder(const QString& path, const int anime_id) {
  QDirIterator it{path, QDir::Dirs, QDirIterator::Subdirectories};

  while (it.hasNext()) {
    const auto info = it.nextFileInfo();

    if (!info.isDir()) continue;

    auto episode = recognition::parseFileInfo(info);

    if (track::recognition::identify(episode) != anime_id) continue;

    return info.filePath();
  }

  return std::nullopt;
}

}  // namespace track
