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

#include "discord.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QTimer>
#include <algorithm>

#include "base/string.hpp"
#include "media/anime_db.hpp"
#include "media/anime_utils.hpp"
#include "sync/service.hpp"
#include "taiga/settings.hpp"
#include "track/media.hpp"

namespace share::discord {

namespace {

constexpr auto kApplicationId = "379871385176244224";
constexpr auto kDefaultImage = "default";  // asset of the application

QString formatEpisode(const track::Episode& episode, const int episodeCount) {
  const auto range = episode.episodeNumberRange();
  if (!range) return {};

  const auto number = range->first == range->second
                          ? QString::number(range->first)
                          : u"%1-%2"_s.arg(range->first).arg(range->second);

  return episodeCount > 0 ? u"Episode %1/%2"_s.arg(number).arg(episodeCount)
                          : u"Episode %1"_s.arg(number);
}

}  // namespace

Presence::Presence(QObject* parent)
    : QObject{parent},
      activityManager_{new ActivityManager{QString::fromLatin1(kApplicationId), this}},
      updateTimer_{new QTimer{this}} {
  // Discord rate-limits activity updates, and players can report changes in quick succession.
  updateTimer_->setSingleShot(true);
  updateTimer_->setInterval(std::chrono::milliseconds{1500});
  connect(updateTimer_, &QTimer::timeout, this, &Presence::update);
}

void Presence::init() {
  connect(track::media::detection(), &track::media::Detection::currentEpisodeChanged, this,
          &Presence::onEpisodeChanged);

  const auto refreshIfCurrent = [this](const int id) {
    if (episode_ && episode_->animeId() == id) updateTimer_->start();
  };
  connect(&anime::db, &anime::Database::itemUpdated, this, refreshIfCurrent);
  connect(&anime::db, &anime::Database::entryUpdated, this, refreshIfCurrent);

  applySettings();
}

void Presence::applySettings() {
  if (taiga::settings.sharingEnabled() && taiga::settings.discordEnabled()) {
    activityManager_->open();
  } else {
    activityManager_->close();
  }
}

void Presence::onEpisodeChanged(const std::optional<track::Episode>& episode) {
  // Identifying the same file as a different anime is not a new viewing.
  const auto isSameFile = [](const track::Episode& a, const track::Episode& b) {
    return std::ranges::equal(a.elements(), b.elements(), [](const auto& x, const auto& y) {
      return x.kind == y.kind && x.value == y.value;
    });
  };
  if (episode && (!episode_ || !isSameFile(*episode_, *episode))) {
    startTime_ = QDateTime::currentSecsSinceEpoch();
  }

  episode_ = episode;
  updateTimer_->start();
}

void Presence::update() {
  if (const auto activity = episode_ ? buildActivity(*episode_, startTime_) : std::nullopt) {
    activityManager_->setActivity(*activity);
  } else {
    activityManager_->clearActivity();
  }
}

Presence* presence() {
  static auto instance = new Presence{qApp};
  return instance;
}

std::optional<Activity> buildActivity(const track::Episode& episode, const qint64 startTime) {
  const auto* item = anime::db.item(episode.animeId());
  if (!item) return std::nullopt;

  if (const auto* entry = anime::db.entry(item->id); entry && entry->is_private) {
    return std::nullopt;
  }

  const auto serviceId = sync::currentServiceId();
  const auto title = QString::fromStdString(anime::preferredTitle(*item));
  const auto imageUrl = QString::fromStdString(item->image_url);
  const auto pageUrl = sync::animePageUrl(item->id);

  return Activity{
      .type = ActivityType::Watching,
      .statusDisplayType = StatusDisplayType::Details,  // "Watching <title>"
      .details = title,
      .detailsUrl = pageUrl,
      .state = formatEpisode(episode, item->episode_count),
      .startTime = startTime,
      .largeImage =
          imageUrl.startsWith(u"https://") ? imageUrl : QString::fromLatin1(kDefaultImage),
      .largeText = title,
      .largeUrl = pageUrl,
      .smallImage = sync::serviceSlug(serviceId),
      .smallText = sync::serviceName(serviceId),
      .buttons = {{.label = u"View Anime"_s, .url = pageUrl}},
  };
}

}  // namespace share::discord
