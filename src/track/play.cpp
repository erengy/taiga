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

#include "media/anime_db.hpp"
#include "taiga/settings.hpp"
#include "track/scanner.hpp"

namespace track {

bool playEpisode(int animeId, int number) {
  const auto libraryFolders = taiga::settings.libraryFolders();

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

bool playNextEpisode(int animeId) {
  const auto item = anime::db.item(animeId);
  if (!item) return false;

  const auto entry = anime::db.entry(animeId);
  const int watched_episodes = entry ? entry->watched_episodes : 0;

  int number = watched_episodes + 1;

  // Replay first episode for completed anime.
  if (item->episode_count > 0 && number > item->episode_count) {
    number = 1;
  }

  return playEpisode(animeId, number);
}

bool playRandomEpisode(int animeId) {
  const auto item = anime::db.item(animeId);
  if (!item) return false;

  const auto entry = anime::db.entry(animeId);
  const int watched_episodes = entry ? entry->watched_episodes : 0;

  int max_episode = item->episode_count;

  // Avoid spoiling unwatched episodes, unless the series is completed.
  // `watched_episodes < episode_count` can be true if rewatching.
  const bool completed = entry && entry->status == anime::list::Status::Completed;
  if (!completed) {
    max_episode = watched_episodes + 1;
  }

  // Clamp to the known episode count, if any.
  if (item->episode_count > 0) {
    max_episode = std::min(max_episode, item->episode_count);
  }

  if (max_episode < 1) return false;

  const int number = QRandomGenerator::global()->bounded(1, max_episode + 1);

  return playEpisode(animeId, number);
}

}  // namespace track
