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

#include "media.hpp"

#include <algorithm>

#include "media/anime.hpp"
#include "media/anime_db.hpp"
#include "taiga/settings.hpp"
#include "track/episode.hpp"
#include "track/media_player.hpp"
#include "track/recognition.hpp"

namespace track::media {

Detection::Detection(QObject* parent) : QObject(parent) {
  pollTimer_ = new QTimer(this);
  connect(pollTimer_, &QTimer::timeout, this, &Detection::poll);
}

const std::optional<Episode> Detection::getCurrentEpisode() const {
  return currentEpisode_;
}

const std::optional<Detection::media_t> Detection::getCurrentMedia() const {
  return currentMedia_;
}

const std::optional<Detection::player_t> Detection::getCurrentPlayer() const {
  return currentPlayer_;
}

bool Detection::init() {
  if (!parsePlayersData(players_)) {
    return false;
  }

#ifdef Q_OS_WINDOWS
  const auto interval = taiga::settings.mediaDetectionInterval();
  pollTimer_->start(interval);
#endif

  return true;
}

void Detection::poll() {
#ifdef Q_OS_WINDOWS
  const auto players = getEnabledPlayers(players_);

  static const auto media_proc = [](const anisthesia::MediaInfo&) {
    return true;  // Accept all media
  };

  std::vector<anisthesia::win::Result> results;
  if (!anisthesia::win::GetResults(players, media_proc, results)) {
    reset();
    return;
  }

  const auto resultIt = std::ranges::find_if(results, [this](const anisthesia::win::Result& r) {
    return r.window.handle == currentWindowHandle_;
  });
  const auto& result = resultIt != results.end() ? *resultIt : results.front();

  currentPlayer_ = result.player;
  currentMedia_ = result.media.front();
  currentWindowHandle_ = result.window.handle;

  const auto mediaInfo = currentMedia_->information.front();
  auto episode = [&mediaInfo]() {
    if (mediaInfo.type == anisthesia::MediaInfoType::File) {
      const QFileInfo fileInfo{QString::fromStdString(mediaInfo.value)};
      return track::recognition::parseFileInfo(fileInfo);
    } else {
      return track::recognition::parse(mediaInfo.value);
    }
  }();

  if (mediaInfo.type == anisthesia::MediaInfoType::File &&
      !track::recognition::isVideoFile(episode)) {
    reset();
    return;
  }

  const auto animeId = track::recognition::identify(episode);
  episode.setAnimeId(animeId);

  if (hasEpisodeChanged(episode)) {
    currentEpisode_ = episode;
    emit currentEpisodeChanged(episode);
  }
#endif
}

bool Detection::isMediaIdentified() const {
  return currentEpisode_.has_value() && currentEpisode_->animeId() != anime::kUnknownId;
}

void Detection::setCurrentEpisodeAnimeId(int animeId) {
  if (!currentEpisode_) return;

  currentEpisode_->setAnimeId(animeId);
  emit currentEpisodeChanged(currentEpisode_);
}

void Detection::reset() {
  currentPlayer_.reset();
  currentMedia_.reset();
  currentWindowHandle_ = nullptr;

  if (currentEpisode_) {
    currentEpisode_.reset();
    emit currentEpisodeChanged(std::nullopt);
  }
}

bool Detection::hasEpisodeChanged(const Episode& episode) const {
  if (!currentEpisode_) return true;
  if (currentEpisode_->animeId() != episode.animeId()) return true;

  return currentEpisode_->elements(anitomy::ElementKind::Episode) !=
         episode.elements(anitomy::ElementKind::Episode);
}

}  // namespace track::media
