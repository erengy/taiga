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

#include "http.hpp"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

#include "base/log.hpp"
#include "base/string.hpp"
#include "media/anime_db.hpp"
#include "taiga/network.hpp"
#include "taiga/settings.hpp"
#include "track/media.hpp"
#include "track/update_decision.hpp"
#include "track/update_session.hpp"

namespace share::http {

namespace {

bool isEnabled() {
  return taiga::settings()->sharingEnabled() && taiga::settings()->webhookEnabled();
}

std::optional<QUrl> targetUrl() {
  QUrl url{QString::fromStdString(taiga::settings()->webhookUrl()), QUrl::StrictMode};
  if (!url.isValid() || url.host().isEmpty()) return std::nullopt;
  if (url.scheme() != u"http" && url.scheme() != u"https") return std::nullopt;
  return url;
}

QHttpHeaders requestHeaders() {
  auto headers = taiga::NetworkAccessManager::commonHeaders();
  headers.replaceOrAppend(QHttpHeaders::WellKnownHeader::ContentType, "application/json");

  // User-defined headers are in "Name: value" format, and may override the common ones.
  for (const auto& header : taiga::settings()->webhookHeaders()) {
    const auto line = QString::fromStdString(header);
    const auto separator = line.indexOf(u':');
    if (separator <= 0) continue;
    const auto name = line.first(separator).trimmed();
    const auto value = line.sliced(separator + 1).trimmed();
    if (!headers.replaceOrAppend(name, value)) {
      qWarning() << "HTTP sharing: invalid header" << name;
    }
  }

  return headers;
}

}  // namespace

Webhook::Webhook(QObject* parent) : QObject{parent}, playingTimer_{new QTimer{this}} {
  // Media players can report changes in quick succession.
  playingTimer_->setSingleShot(true);
  playingTimer_->setInterval(std::chrono::milliseconds{1500});
  connect(playingTimer_, &QTimer::timeout, this, &Webhook::updatePlaying);
}

void Webhook::init() {
  connect(track::media::detection(), &track::media::Detection::currentEpisodeChanged, this,
          &Webhook::onEpisodeChanged);
  connect(track::updateSession(), &track::UpdateSession::stateChanged, this,
          &Webhook::onUpdateStateChanged);

  // The anime may become shareable or not (e.g. the list entry is made private).
  const auto refreshIfCurrent = [this](const int id) {
    if (episode_ && episode_->animeId() == id) playingTimer_->start();
  };
  connect(anime::db(), &anime::Database::itemUpdated, this, refreshIfCurrent);
  connect(anime::db(), &anime::Database::entryUpdated, this, refreshIfCurrent);

  connect(taiga::settings(), &taiga::Settings::changed, this, &Webhook::onSettingsChanged);
}

void Webhook::applySettings() {
  updatePlaying();
}

void Webhook::onSettingsChanged(const QSet<QString>& keys) {
  static const QSet<QString> relevantKeys{
      taiga::Settings::Key::kSharingEnabled,
      taiga::Settings::Key::kWebhookEnabled,
      taiga::Settings::Key::kWebhookHeaders,
      taiga::Settings::Key::kWebhookUrl,
  };
  if (keys.intersects(relevantKeys)) applySettings();
}

void Webhook::onEpisodeChanged(const std::optional<track::Episode>& episode) {
  episode_ = episode;
  playingTimer_->start();
}

void Webhook::onUpdateStateChanged(const track::UpdateState& state) {
  if (state.phase != track::UpdateState::Phase::Committed) return;
  if (!isEnabled() || !shareableAnime(state.animeId)) return;

  send({
      .type = EventType::Updated,
      .animeId = state.animeId,
      .episode = state.episode,
  });
}

void Webhook::updatePlaying() {
  playingTimer_->stop();

  std::optional<Event> next;
  if (isEnabled() && episode_) {
    if (const auto* item = shareableAnime(episode_->animeId())) {
      next = Event{
          .type = EventType::Playing,
          .animeId = item->id,
          .episode = track::watchedEpisodeNumber(*episode_, *item).value_or(0),
      };
    }
  }

  const auto isSamePlaying = [](const Event& a, const Event& b) {
    return a.animeId == b.animeId && a.episode == b.episode;
  };
  if (next && playing_ && isSamePlaying(*next, *playing_)) return;

  if (next) {
    send(*next);  // replaces the previous one, if any
  } else if (playing_) {
    send({
        .type = EventType::Stopped,
        .animeId = playing_->animeId,
        .episode = playing_->episode,
    });
  }

  playing_ = next;
}

void Webhook::send(const Event& event) const {
  const auto url = targetUrl();
  if (!url) return;

  QNetworkRequest request{*url};
  request.setHeaders(requestHeaders());

  const auto data = QJsonDocument{toJson(event)}.toJson(QJsonDocument::Compact);
  auto* reply = taiga::network()->post(request, data);

  connect(reply, &QNetworkReply::finished, reply, [reply]() {
    if (reply->error() != QNetworkReply::NoError) {
      qWarning() << "HTTP sharing:" << reply->errorString();
    }
  });
}

Webhook* webhook() {
  static auto instance = new Webhook{qApp};
  return instance;
}

}  // namespace share::http
