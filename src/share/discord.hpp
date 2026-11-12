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

#pragma once

#include <QObject>
#include <optional>

#include "share/discord_rpc.hpp"
#include "track/episode.hpp"

class QTimer;

namespace share::discord {

// Shares the episode being watched as Discord rich presence.
class Presence final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Presence)

public:
  Presence(QObject* parent);
  ~Presence() override = default;

  void init();

  // Connects or disconnects according to the sharing settings, which have no change
  // notifications. Disconnecting removes the presence immediately.
  void applySettings();

private:
  void onEpisodeChanged(const std::optional<track::Episode>& episode);
  void update();

  ActivityManager* activityManager_ = nullptr;
  QTimer* updateTimer_ = nullptr;
  std::optional<track::Episode> episode_;
  qint64 startTime_ = 0;
};

Presence* presence();

// Activity for the episode being watched, or nothing if it should not be shared (unknown or
// private anime). `startTime` (seconds since epoch) is shown as elapsed time.
std::optional<Activity> buildActivity(const track::Episode& episode, qint64 startTime);

}  // namespace share::discord
