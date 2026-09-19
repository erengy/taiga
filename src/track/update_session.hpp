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

#include <QApplication>
#include <QObject>
#include <QTimer>
#include <chrono>
#include <optional>

#include "track/episode.hpp"
#include "track/update_state.hpp"
#include "track/update_trigger.hpp"

namespace track {

class UpdateSession final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(UpdateSession)

public:
  UpdateSession(QObject* parent);

  void init();

  const UpdateState& state() const;

  void accept();
  void cancel();

signals:
  void stateChanged(const UpdateState& state) const;

private:
  void onEpisodeChanged(std::optional<Episode> episode);
  void onMediaClosed();
  void tick();

  void evaluate();
  void commit(const Anime& item);
  void reset();
  void setState(const UpdateState& state);

  std::optional<Episode> episode_;
  std::chrono::seconds elapsed_{0};
  std::chrono::seconds delay_{0};
  UpdateTrigger trigger_ = UpdateTrigger::AfterDelay;
  bool pauseWhenUnfocused_ = false;
  bool paused_ = false;
  bool dismissed_ = false;
  bool committed_ = false;

  UpdateState state_;
  QTimer* timer_;
};

inline UpdateSession* updateSession() {
  static auto session = new UpdateSession(qApp);
  return session;
}

}  // namespace track
