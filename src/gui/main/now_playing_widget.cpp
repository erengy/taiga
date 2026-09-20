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

#include <QApplication>
#include <QBoxLayout>
#include <QDesktopServices>
#include <QLabel>
#include <QUrl>
#include <optional>
#include <utility>

#include "base/chrono.hpp"
#include "base/string.hpp"
#include "gui/media/media_dialog.hpp"
#include "gui/media/media_menu.hpp"
#include "gui/utils/format.hpp"
#include "gui/utils/image_provider.hpp"
#include "gui/utils/theme.hpp"
#include "media/anime_db.hpp"
#include "media/anime_list.hpp"
#include "media/anime_utils.hpp"
#include "sync/service.hpp"
#include "track/episode.hpp"
#include "track/media.hpp"
#include "track/update_session.hpp"

namespace gui {

namespace {

constexpr int kPosterHeight = 64;
constexpr int kPosterWidth = kPosterHeight * 2 / 3;
constexpr int kPosterCornerRadius = 4;
constexpr int kHorizontalMargin = 16;
constexpr int kVerticalMargin = 12;
constexpr int kSpacing = 12;

QString formatUpdateState(const track::UpdateState& state) {
  using Phase = track::UpdateState::Phase;
  using Reason = track::UpdateDecision::Reason;

  switch (state.phase) {
    case Phase::Countdown:
      return u"List update in %1%2"_s.arg(formatDuration(Duration{state.remaining}))
          .arg(state.paused ? u" (paused)"_s : QString{});

    case Phase::WaitingForClose:
      return u"List will be updated when the media is closed"_s;

    case Phase::Denied:
      switch (state.reason) {
        case Reason::AlreadyWatched:
          return u"List won't be updated: episode already watched"_s;
        case Reason::InvalidEpisode:
          return u"List won't be updated: invalid episode number"_s;
        case Reason::OutsideLibrary:
          return u"List won't be updated: file is outside of library folders"_s;
        default:
          return {};
      }

    case Phase::Confirming:
      return u"Episode %1 is ahead of your progress (%2)"_s.arg(state.episode)
          .arg(state.previousEpisode);

    case Phase::Committed:
      return u"List updated: episode %1 → %2"_s.arg(state.previousEpisode).arg(state.episode);

    case Phase::Cancelled:
      return u"List update cancelled"_s;

    default:
      return {};
  }
}

std::pair<QString, QString> formatUpdateActions(const track::UpdateState& state) {
  using Phase = track::UpdateState::Phase;

  switch (state.phase) {
    case Phase::Countdown:
    case Phase::WaitingForClose:
      return {u"Update now"_s, u"Cancel"_s};

    case Phase::Confirming:
      return {u"Update to %1"_s.arg(state.episode), u"Ignore"_s};

    case Phase::Cancelled:
      return {u"Update"_s, {}};

    default:
      return {};
  }
}

QColor statusColor(const track::UpdateState& state) {
  using Phase = track::UpdateState::Phase;

  switch (state.phase) {
    case Phase::Committed:
      return Theme::successColor();

    case Phase::Confirming:
    case Phase::Denied:
      return Theme::warningColor();

    default:
      return {};
  }
}

QString statusIconName(const track::UpdateState& state, const bool isRecognized) {
  using Phase = track::UpdateState::Phase;

  switch (state.phase) {
    case Phase::Countdown:
    case Phase::WaitingForClose:
      return u"sync"_s;

    case Phase::Confirming:
      return u"help"_s;

    case Phase::Denied:
    case Phase::Cancelled:
      return u"info"_s;

    case Phase::Committed:
      return u"check_circle"_s;

    default:
      return isRecognized ? QString{} : u"error"_s;
  }
}

}  // namespace

NowPlayingWidget::NowPlayingWidget(QWidget* parent) : QFrame(parent) {
  setObjectName("nowPlaying");
  setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);
  setFixedHeight(kPosterHeight + 2 * kVerticalMargin);

  const auto layout = new QHBoxLayout(this);
  layout->setContentsMargins(kHorizontalMargin, kVerticalMargin, kHorizontalMargin,
                             kVerticalMargin);
  layout->setSpacing(kSpacing);

  // Poster
  m_posterWidget = new PosterWidget(this);
  m_posterWidget->setFixedSize(kPosterWidth, kPosterHeight);
  m_posterWidget->setCornerRadius(kPosterCornerRadius);
  layout->addWidget(m_posterWidget);
  connect(m_posterWidget, &PosterWidget::clicked, this, [this](Qt::MouseButton button) {
    if (button == Qt::MouseButton::LeftButton && m_anime) {
      QDesktopServices::openUrl(QUrl{sync::animePageUrl(m_anime->id)});
    }
  });

  // Text
  const auto textLayout = new QVBoxLayout();
  textLayout->setSpacing(2);
  layout->addLayout(textLayout, 1);
  textLayout->addStretch();

  m_titleLabel = new ClickableLabel(this);
  m_titleLabel->setElidable(true);
  auto titleFont = m_titleLabel->font();
  titleFont.setWeight(QFont::DemiBold);
  m_titleLabel->setFont(titleFont);
  textLayout->addWidget(m_titleLabel);
  connect(m_titleLabel, &ClickableLabel::clicked, this, [this](Qt::MouseButton button) {
    if (button == Qt::LeftButton && m_anime) {
      MediaDialog::show(this, MediaDialogPage::Details, *m_anime);
    }
  });
  m_titleLabel->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_titleLabel, &QLabel::customContextMenuRequested, this, [this]() {
    if (!m_anime) return;
    QMap<int, ListEntry> entries;
    if (const auto entry = anime::db.entry(m_anime->id)) {
      entries[m_anime->id] = *entry;
    }
    auto* menu = new MediaMenu(this, {*m_anime}, entries, nullptr, AnimeListContext::List);
    menu->popup();
  });

  m_detailsLabel = new ClickableLabel(this);
  m_detailsLabel->setElidable(true);
  m_detailsLabel->setForegroundRole(QPalette::PlaceholderText);
  textLayout->addWidget(m_detailsLabel);

  const auto statusLayout = new QHBoxLayout();
  statusLayout->setSpacing(4);
  textLayout->addLayout(statusLayout);
  textLayout->addStretch();

  m_iconLabel = new QLabel(this);
  m_iconLabel->setFixedSize(16, 16);
  statusLayout->addWidget(m_iconLabel);

  m_statusLabel = new ClickableLabel(this);
  m_statusLabel->setElidable(true);
  statusLayout->addWidget(m_statusLabel, 1);

  // Actions
  m_actions = new QWidget(this);
  layout->addWidget(m_actions);

  const auto actionsLayout = new QVBoxLayout(m_actions);
  actionsLayout->setContentsMargins(0, 0, 0, 0);
  actionsLayout->setSpacing(4);
  actionsLayout->addStretch();

  m_acceptButton = new QPushButton(m_actions);
  m_acceptButton->setDefault(true);
  actionsLayout->addWidget(m_acceptButton);
  connect(m_acceptButton, &QPushButton::clicked, this, []() { track::updateSession()->accept(); });

  m_cancelButton = new QPushButton(m_actions);
  actionsLayout->addWidget(m_cancelButton);
  connect(m_cancelButton, &QPushButton::clicked, this, []() { track::updateSession()->cancel(); });

  actionsLayout->addStretch();

  refresh();

  connect(&anime::db, &anime::Database::itemUpdated, this, [this](const int id) {
    if (!m_anime || m_anime->id != id) return;
    if (const auto item = anime::db.item(id)) m_anime = *item;
    refresh();
  });
  connect(&imageProvider, &ImageProvider::posterChanged, this, [this](const int id) {
    if (m_anime && m_anime->id == id) refresh();
  });
  connect(track::updateSession(), &track::UpdateSession::stateChanged, this, [this]() {
    refresh();
    updateVisibility();
  });
  connect(track::media::detection(), &track::media::Detection::currentEpisodeChanged, this,
          [this](std::optional<track::Episode> episode) {
            if (episode) {
              setPlaying(*episode);
            } else {
              refresh();
            }
            updateVisibility();
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

void NowPlayingWidget::updateVisibility() {
  const bool isPlaying = track::media::detection()->getCurrentEpisode().has_value();
  const bool hasUpdate = track::updateSession()->state().phase != track::UpdateState::Phase::Idle;
  if (!isPlaying && !hasUpdate) reset();
}

void NowPlayingWidget::refresh() {
  if (!m_episode.has_value()) {
    render(std::nullopt);
    return;
  }

  QStringList lines;
  if (const auto player = track::media::detection()->getCurrentPlayer()) {
    lines += u"<b>Media player:</b> %1"_s.arg(player->name);
  }
  if (m_episode->contains(anitomy::ElementKind::EpisodeTitle)) {
    const auto episodeTitle = m_episode->element(anitomy::ElementKind::EpisodeTitle);
    lines += u"<b>Episode title:</b> %1"_s.arg(episodeTitle);
  }
  if (m_episode->contains(anitomy::ElementKind::ReleaseGroup)) {
    const auto releaseGroup = m_episode->element(anitomy::ElementKind::ReleaseGroup);
    lines += u"<b>Group:</b> %1"_s.arg(releaseGroup);
  }

  const auto title =
      m_anime ? anime::preferredTitle(*m_anime) : m_episode->element(anitomy::ElementKind::Title);
  const auto episodeNumber =
      formatEpisodeNumbers(m_episode->elements(anitomy::ElementKind::Episode));
  const auto episodeCount = formatNumber(m_anime ? m_anime->episode_count : 0, "?");

  QPixmap poster;
  if (m_anime) poster = imageProvider.loadPoster(m_anime->id);

  render(Content{
      .title = QString::fromStdString(title),
      .progress = u"%1/%2"_s.arg(episodeNumber).arg(episodeCount),
      .episodeTitle =
          QString::fromStdString(m_episode->element(anitomy::ElementKind::EpisodeTitle)),
      .details = lines.join("<br>"),
      .isPlaying = track::media::detection()->getCurrentEpisode().has_value(),
      .isRecognized = m_anime.has_value(),
      .poster = poster,
      .isPosterLoading = m_anime && poster.isNull() && !m_anime->image_url.empty(),
      .updateState = track::updateSession()->state(),
  });
}

void NowPlayingWidget::render(const std::optional<Content>& content) {
  if (!content) {
    setToolTip({});
    m_posterWidget->setPixmap({});
    m_posterWidget->setLoading(false);
    m_titleLabel->setText({});
    m_detailsLabel->setText({});
    m_iconLabel->clear();
    m_statusLabel->setText({});
    m_actions->hide();
    return;
  }

  setToolTip(content->details);

  m_posterWidget->setPixmap(content->poster);
  m_posterWidget->setLoading(content->isPosterLoading);
  m_posterWidget->setCursor(content->isRecognized ? Qt::PointingHandCursor : Qt::ArrowCursor);

  m_titleLabel->setText(content->title);
  m_titleLabel->setCursor(content->isRecognized ? Qt::PointingHandCursor : Qt::ArrowCursor);

  auto details = u"%1 · Episode %2"_s.arg(content->isPlaying ? u"Watching"_s : u"Watched"_s)
                     .arg(content->progress);
  if (!content->episodeTitle.isEmpty()) details += u" · "_s + content->episodeTitle;
  m_detailsLabel->setText(details);

  if (const auto iconName = statusIconName(content->updateState, content->isRecognized);
      iconName.isEmpty()) {
    m_iconLabel->clear();
  } else {
    m_iconLabel->setPixmap(theme.getIcon(iconName).pixmap(QSize(16, 16)));
  }
  m_statusLabel->setText(formatUpdateState(content->updateState));

  if (const auto color = statusColor(content->updateState); color.isValid()) {
    auto palette = QApplication::palette();
    palette.setColor(QPalette::WindowText, color);
    m_statusLabel->setPalette(palette);
    m_statusLabel->setForegroundRole(QPalette::WindowText);
  } else {
    m_statusLabel->setPalette({});
    m_statusLabel->setForegroundRole(QPalette::PlaceholderText);
  }

  const auto [acceptText, cancelText] = formatUpdateActions(content->updateState);
  m_acceptButton->setText(acceptText);
  m_acceptButton->setVisible(!acceptText.isEmpty());
  m_cancelButton->setText(cancelText);
  m_cancelButton->setVisible(!cancelText.isEmpty());
  m_actions->setVisible(!acceptText.isEmpty());
}

}  // namespace gui
