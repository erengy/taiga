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

#include "now_playing_widget.hpp"

#include <QBoxLayout>
#include <QLabel>
#include <optional>

#include "base/string.hpp"
#include "gui/media/media_dialog.hpp"
#include "gui/utils/format.hpp"
#include "gui/utils/theme.hpp"
#include "media/anime_db.hpp"
#include "media/anime_utils.hpp"
#include "track/episode.hpp"
#include "track/media.hpp"

namespace gui {

NowPlayingWidget::NowPlayingWidget(QWidget* parent) : QFrame(parent) {
  setObjectName("nowPlaying");
  setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Maximum);

  const auto layout = new QHBoxLayout(this);
  layout->setContentsMargins(16, 16, 16, 16);

  // Icon
  m_iconLabel = new QLabel(this);
  m_iconLabel->setFixedWidth(16);
  m_iconLabel->setFixedHeight(16);
  m_iconLabel->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
  m_iconLabel->setPixmap(theme.getIcon("info").pixmap(QSize(16, 16)));
  layout->addWidget(m_iconLabel);

  // Main
  m_mainLabel = new QLabel(this);
  layout->addWidget(m_mainLabel);
  connect(m_mainLabel, &QLabel::linkActivated, this, [this]() {
    if (m_anime) {
      m_mainLabel->unsetCursor();
      MediaDialog::show(this, MediaDialogPage::Details, *m_anime);
    }
  });

  // Timer
  m_timerLabel = new QLabel(this);
  m_timerLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  layout->addWidget(m_timerLabel);

  refresh();

  connect(track::media::detection(), &track::media::Detection::currentEpisodeChanged, this,
          [this](std::optional<track::Episode> episode) {
            if (episode) {
              setPlaying(*episode);
            } else {
              reset();
            }
          });
}

void NowPlayingWidget::reset() {
  hide();
  m_anime.reset();
  m_episode.reset();
  refresh();
}

void NowPlayingWidget::setPlaying(track::Episode episode) {
  m_episode = episode;

  if (const auto item = anime::db.item(episode.animeId())) {
    m_anime = *item;
  } else {
    m_anime.reset();
  }

  refresh();
  show();
}

void NowPlayingWidget::refresh() {
  if (!m_episode.has_value()) {
    m_iconLabel->setToolTip({});
    m_mainLabel->setText({});
    m_timerLabel->setText({});
    return;
  }

  QStringList lines;
  if (const auto player = track::media::detection()->getCurrentPlayer()) {
    lines += u"<b>Media player:</b> %1"_s.arg(QString::fromStdString(player->name));
  }
  if (m_episode->contains(anitomy::ElementKind::EpisodeTitle)) {
    const auto episodeTitle = m_episode->element(anitomy::ElementKind::EpisodeTitle);
    lines += u"<b>Episode title:</b> %1"_s.arg(episodeTitle);
  }
  if (m_episode->contains(anitomy::ElementKind::ReleaseGroup)) {
    const auto releaseGroup = m_episode->element(anitomy::ElementKind::ReleaseGroup);
    lines += u"<b>Group:</b> %1"_s.arg(releaseGroup);
  }
  m_iconLabel->setToolTip(lines.join("<br>"));

  const QString iconName = m_anime ? "check_circle" : "info";
  m_iconLabel->setPixmap(theme.getIcon(iconName).pixmap(QSize(16, 16)));

  const auto title =
      m_anime ? anime::preferredTitle(*m_anime) : m_episode->element(anitomy::ElementKind::Title);
  const auto episodeNumber = m_episode->element(anitomy::ElementKind::Episode, "1");
  const auto episodeCount = formatNumber(m_anime ? m_anime->episode_count : 0, "?");

  m_mainLabel->setText(u"Watching <a href=\"#\" style=\"%3\">%1</a> – Episode %2"_s
                           .arg(QString::fromStdString(title))
                           .arg(u"%1/%2"_s.arg(episodeNumber).arg(episodeCount))
                           .arg("font-weight: 600; text-decoration: none;"));

  m_timerLabel->setText("List update in <b style=\"font-weight: 600;\">00:00</b>");
}

}  // namespace gui
