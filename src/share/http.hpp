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
#include <QSet>
#include <optional>

#include "share/event.hpp"
#include "track/episode.hpp"
#include "track/update_state.hpp"

class QTimer;

namespace share::http {

// Sends sharing events as JSON in POST requests to a user-defined URL. See `share::toJson` for
// the format.
class Webhook final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Webhook)

public:
  Webhook(QObject* parent);
  ~Webhook() override = default;

  void init();

private:
  // Sends `stopped` if sharing was disabled while an episode was shared as playing.
  void applySettings();
  void onSettingsChanged(const QSet<QString>& keys);
  void onEpisodeChanged(const std::optional<track::Episode>& episode);
  void onUpdateStateChanged(const track::UpdateState& state);
  void updatePlaying();

  void send(const Event& event) const;

  QTimer* playingTimer_ = nullptr;
  std::optional<track::Episode> episode_;
  std::optional<Event> playing_;  // last `playing` event that was sent
};

Webhook* webhook();

}  // namespace share::http
