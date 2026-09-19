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

#include "update_session.hpp"

#include "base/log.hpp"
#include "media/anime.hpp"
#include "media/anime_db.hpp"
#include "media/anime_list_utils.hpp"
#include "taiga/settings.hpp"
#include "track/media.hpp"
#include "track/update_decision.hpp"

namespace track {

namespace {

bool isSameMedia(const Episode& previous, const Episode& current) {
  if (previous.animeId() != current.animeId()) {
    // Identifying the anime doesn't make it a new one.
    if (previous.animeId() != anime::kUnknownId) return false;
  }

  return previous.elements(anitomy::ElementKind::Episode) ==
         current.elements(anitomy::ElementKind::Episode);
}

}  // namespace

UpdateSession::UpdateSession(QObject* parent) : QObject(parent) {
  timer_ = new QTimer(this);
  timer_->setInterval(std::chrono::seconds{1});
  timer_->setTimerType(Qt::PreciseTimer);
  connect(timer_, &QTimer::timeout, this, &UpdateSession::tick);
}

void UpdateSession::init() {
  connect(media::detection(), &media::Detection::currentEpisodeChanged, this,
          &UpdateSession::onEpisodeChanged);
}

const UpdateState& UpdateSession::state() const {
  return state_;
}

void UpdateSession::accept() {
  if (!episode_ || committed_) return;

  const auto* item = anime::db.item(episode_->animeId());
  if (!item) return;

  const auto* entry = anime::db.entry(item->id);
  const auto decision = decideUpdate(*episode_, *item, entry);
  if (decision.action == UpdateDecision::Action::Deny) return;

  commit(*item);
}

void UpdateSession::cancel() {
  if (!episode_ || committed_) return;

  dismissed_ = true;
  evaluate();
}

void UpdateSession::onEpisodeChanged(std::optional<Episode> episode) {
  if (!episode) {
    onMediaClosed();
    return;
  }

  if (!episode_ || !isSameMedia(*episode_, *episode)) {
    elapsed_ = std::chrono::seconds{0};
    delay_ = taiga::settings.updateDelay();
    trigger_ = taiga::settings.updateTrigger();
    pauseWhenUnfocused_ = taiga::settings.updatePauseWhenUnfocused();
    paused_ = false;
    dismissed_ = false;
    committed_ = false;
    notified_ = false;
  }

  episode_ = std::move(episode);

  if (!committed_ && !timer_->isActive()) timer_->start();

  evaluate();
}

void UpdateSession::onMediaClosed() {
  timer_->stop();

  evaluate();

  // Committed state stays in place until the next media.
  if (committed_) return;

  switch (state_.phase) {
    case UpdateState::Phase::WaitingForClose:
      if (const auto* item = anime::db.item(episode_->animeId())) {
        commit(*item);
        return;
      }
      break;
    case UpdateState::Phase::Confirming:
      // Keep asking, unless the media was closed too soon.
      if (elapsed_ >= delay_) return;
      break;
    default:
      break;
  }

  reset();
}

void UpdateSession::tick() {
  // We count ticks instead of comparing timestamps, so that skipping a tick is enough to pause.
  // Being out of focus is our best guess for the media player not playing.
  paused_ = pauseWhenUnfocused_ && !media::detection()->isPlayerFocused();
  if (!paused_) elapsed_ += std::chrono::seconds{1};
  evaluate();
}

void UpdateSession::evaluate() {
  if (committed_) return;

  const auto* item = episode_ ? anime::db.item(episode_->animeId()) : nullptr;
  if (!item) {
    setState({});
    return;
  }

  const auto* entry = anime::db.entry(item->id);
  const auto decision = decideUpdate(*episode_, *item, entry);

  using Action = UpdateDecision::Action;
  using Phase = UpdateState::Phase;

  UpdateState state{
      .reason = decision.reason,
      .episode = watchedEpisodeNumber(*episode_, *item).value_or(0),
      .previousEpisode = anime::list::isInList(entry) ? entry->watched_episodes : 0,
  };

  switch (decision.action) {
    case Action::Allow:
      if (dismissed_) {
        state.phase = Phase::Cancelled;
      } else if (const auto remaining = delay_ - elapsed_; remaining > std::chrono::seconds{0}) {
        state.phase = Phase::Countdown;
        state.remaining = remaining;
        state.paused = paused_;
      } else if (trigger_ == UpdateTrigger::OnPlayerClose) {
        state.phase = Phase::WaitingForClose;
      } else {
        commit(*item);
        return;
      }
      break;
    case Action::Confirm:
      state.phase = dismissed_ ? Phase::Cancelled : Phase::Confirming;
      break;
    case Action::Deny:
      state.phase = Phase::Denied;
      break;
  }

  setState(state);

  if (state.phase == Phase::Confirming && elapsed_ >= delay_ && !notified_) {
    notified_ = true;
    emit confirmationRequested(state_);
  }
}

void UpdateSession::commit(const Anime& item) {
  const auto* entry = anime::db.entry(item.id);
  const auto number = watchedEpisodeNumber(*episode_, item);
  if (!number) return;

  const int previous = anime::list::isInList(entry) ? entry->watched_episodes : 0;

  qDebug() << "Updating list:" << item.id << "episode" << *number;
  anime::list::save(anime::list::entryWithEpisodeWatched(item, entry, *number));

  committed_ = true;
  timer_->stop();

  setState({
      .phase = UpdateState::Phase::Committed,
      .episode = *number,
      .previousEpisode = previous,
  });
}

void UpdateSession::reset() {
  episode_.reset();
  dismissed_ = false;
  committed_ = false;
  setState({});
}

void UpdateSession::setState(const UpdateState& state) {
  if (state == state_) return;

  state_ = state;
  emit stateChanged(state_);
}

}  // namespace track
