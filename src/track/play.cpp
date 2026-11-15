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

#include "play.hpp"

#include <QDesktopServices>
#include <QRandomGenerator>
#include <QUrl>

#include "base/log.hpp"
#include "media/anime_db.hpp"
#include "taiga/settings.hpp"
#include "track/scanner.hpp"

namespace track {

bool playEpisode(int animeId, int number) {
  const auto libraryFolders = taiga::settings()->libraryFolders();

  for (const auto& folder : libraryFolders) {
    const auto episodePath = findEpisode(QString::fromStdString(folder), animeId, number);
    if (episodePath) {
      qDebug() << "Found file:" << *episodePath;
      return QDesktopServices::openUrl(QUrl::fromLocalFile(*episodePath));
    }
  }

  qDebug() << "Not found:" << animeId << "episode" << number;

  return false;
}

std::optional<int> nextEpisodeNumber(int animeId) {
  const auto item = anime::db()->item(animeId);
  if (!item) return std::nullopt;

  const auto entry = anime::db()->entry(animeId);
  const int watchedEpisodes = entry ? entry->watchedEpisodes : 0;

  int number = watchedEpisodes + 1;

  // Replay first episode for completed anime.
  if (item->episodeCount > 0 && number > item->episodeCount) {
    number = 1;
  }

  return number;
}

std::optional<int> randomEpisodeNumber(int animeId) {
  const auto item = anime::db()->item(animeId);
  if (!item) return std::nullopt;

  const auto entry = anime::db()->entry(animeId);
  const int watchedEpisodes = entry ? entry->watchedEpisodes : 0;

  int maxEpisode = item->episodeCount;

  // Avoid spoiling unwatched episodes, unless the series is completed.
  // `watchedEpisodes < episodeCount` can be true if rewatching.
  const bool completed = entry && entry->status == anime::list::Status::Completed;
  if (!completed) {
    maxEpisode = watchedEpisodes + 1;
  }

  // Clamp to the known episode count, if any.
  if (item->episodeCount > 0) {
    maxEpisode = std::min(maxEpisode, item->episodeCount);
  }

  if (maxEpisode < 1) return std::nullopt;

  return QRandomGenerator::global()->bounded(1, maxEpisode + 1);
}

}  // namespace track
