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
#include <QDesktopServices>
#include <QEvent>
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
#include "taiga/settings.hpp"
#include "track/episode.hpp"
#include "track/media.hpp"
#include "track/update_session.hpp"

namespace gui {

namespace {

constexpr int kPosterHeight = 64;
constexpr int kPosterWidth = kPosterHeight * 2 / 3;
constexpr int kPosterCornerRadius = 4;
constexpr int kPosterSpinnerSize = 20;
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
      return theme()->colors().success;

    case Phase::Confirming:
    case Phase::Denied:
      return theme()->colors().warning;

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
  posterWidget_ = new PosterWidget(this);
  posterWidget_->setFixedSize(kPosterWidth, kPosterHeight);
  posterWidget_->setCornerRadius(kPosterCornerRadius);
  posterWidget_->setSpinnerSize(kPosterSpinnerSize);
  layout->addWidget(posterWidget_);
  connect(posterWidget_, &PosterWidget::clicked, this, [this](Qt::MouseButton button) {
    if (button == Qt::MouseButton::LeftButton && anime_) {
      QDesktopServices::openUrl(QUrl{sync::animePageUrl(anime_->id)});
    }
  });

  // Text
  const auto textLayout = new QVBoxLayout();
  textLayout->setSpacing(2);
  layout->addLayout(textLayout, 1);
  textLayout->addStretch();

  titleLabel_ = new ClickableLabel(this);
  titleLabel_->setElidable(true);
  auto titleFont = titleLabel_->font();
  titleFont.setWeight(QFont::DemiBold);
  titleLabel_->setFont(titleFont);
  textLayout->addWidget(titleLabel_);
  connect(titleLabel_, &ClickableLabel::clicked, this, [this](Qt::MouseButton button) {
    if (button == Qt::LeftButton && anime_) {
      MediaDialog::show(this, MediaDialogPage::Details, *anime_);
    }
  });
  titleLabel_->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(titleLabel_, &QLabel::customContextMenuRequested, this, [this]() {
    if (!anime_) return;
    QMap<int, ListEntry> entries;
    if (const auto entry = anime::db()->entry(anime_->id)) {
      entries[anime_->id] = *entry;
    }
    auto* menu = new MediaMenu(this, {*anime_}, entries, nullptr, AnimeListContext::List);
    menu->popup();
  });

  detailsLabel_ = new ClickableLabel(this);
  detailsLabel_->setElidable(true);
  detailsLabel_->setForegroundRole(QPalette::PlaceholderText);
  textLayout->addWidget(detailsLabel_);

  const auto statusLayout = new QHBoxLayout();
  statusLayout->setSpacing(4);
  textLayout->addLayout(statusLayout);
  textLayout->addStretch();

  iconLabel_ = new QLabel(this);
  iconLabel_->setFixedSize(16, 16);
  statusLayout->addWidget(iconLabel_);

  statusLabel_ = new ClickableLabel(this);
  statusLabel_->setElidable(true);
  statusLayout->addWidget(statusLabel_, 1);

  // Actions
  actions_ = new QWidget(this);
  layout->addWidget(actions_);

  const auto actionsLayout = new QVBoxLayout(actions_);
  actionsLayout->setContentsMargins(0, 0, 0, 0);
  actionsLayout->setSpacing(4);
  actionsLayout->addStretch();

  acceptButton_ = new QPushButton(actions_);
  acceptButton_->setDefault(true);
  actionsLayout->addWidget(acceptButton_);
  connect(acceptButton_, &QPushButton::clicked, this, []() { track::updateSession()->accept(); });

  cancelButton_ = new QPushButton(actions_);
  actionsLayout->addWidget(cancelButton_);
  connect(cancelButton_, &QPushButton::clicked, this, []() { track::updateSession()->cancel(); });

  actionsLayout->addStretch();

  refresh();

  connect(anime::db(), &anime::Database::itemUpdated, this, [this](const int id) {
    if (!anime_ || anime_->id != id) return;
    if (const auto item = anime::db()->item(id)) anime_ = *item;
    refresh();
  });
  connect(imageProvider(), &ImageProvider::posterChanged, this, [this](const int id) {
    if (anime_ && anime_->id == id) refresh();
  });
  connect(taiga::settings(), &taiga::Settings::changed, this, [this](const QSet<QString>& keys) {
    if (keys.contains(taiga::Settings::Key::kAppTitleLanguage)) refresh();
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

void NowPlayingWidget::changeEvent(QEvent* event) {
  if (event->type() == QEvent::PaletteChange && episode_) refresh();
  QFrame::changeEvent(event);
}

void NowPlayingWidget::reset() {
  hide();
  anime_.reset();
  episode_.reset();
  refresh();
}

void NowPlayingWidget::setPlaying(const track::Episode& episode) {
  episode_ = episode;

  if (const auto item = anime::db()->item(episode.animeId())) {
    anime_ = *item;
  } else {
    anime_.reset();
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
  if (!episode_.has_value()) {
    render(std::nullopt);
    return;
  }

  QStringList lines;
  if (const auto player = track::media::detection()->getCurrentPlayer()) {
    lines += u"<b>Media player:</b> %1"_s.arg(player->name);
  }
  if (episode_->contains(anitomy::ElementKind::EpisodeTitle)) {
    const auto episodeTitle = episode_->element(anitomy::ElementKind::EpisodeTitle);
    lines += u"<b>Episode title:</b> %1"_s.arg(episodeTitle);
  }
  if (episode_->contains(anitomy::ElementKind::ReleaseGroup)) {
    const auto releaseGroup = episode_->element(anitomy::ElementKind::ReleaseGroup);
    lines += u"<b>Group:</b> %1"_s.arg(releaseGroup);
  }

  const auto title =
      anime_ ? anime::preferredTitle(*anime_) : episode_->element(anitomy::ElementKind::Title);
  const auto episodeNumber =
      formatEpisodeNumbers(episode_->elements(anitomy::ElementKind::Episode));
  const auto episodeCount = formatNumber(anime_ ? anime_->episodeCount : 0, "?");

  QPixmap poster;
  if (anime_) poster = imageProvider()->loadPoster(anime_->id);

  render(Content{
      .title = QString::fromStdString(title),
      .progress = u"%1/%2"_s.arg(episodeNumber).arg(episodeCount),
      .episodeTitle = QString::fromStdString(episode_->element(anitomy::ElementKind::EpisodeTitle)),
      .details = lines.join("<br>"),
      .isPlaying = track::media::detection()->getCurrentEpisode().has_value(),
      .isRecognized = anime_.has_value(),
      .poster = poster,
      .isPosterLoading = anime_ && poster.isNull() && !anime_->imageUrl.empty(),
      .updateState = track::updateSession()->state(),
  });
}

void NowPlayingWidget::render(const std::optional<Content>& content) {
  if (!content) {
    setToolTip({});
    posterWidget_->setPixmap({});
    posterWidget_->setLoading(false);
    titleLabel_->setText({});
    detailsLabel_->setText({});
    iconLabel_->clear();
    statusLabel_->setText({});
    actions_->hide();
    return;
  }

  setToolTip(content->details);

  posterWidget_->setPixmap(content->poster);
  posterWidget_->setLoading(content->isPosterLoading);
  posterWidget_->setCursor(content->isRecognized ? Qt::PointingHandCursor : Qt::ArrowCursor);

  titleLabel_->setText(content->title);
  titleLabel_->setCursor(content->isRecognized ? Qt::PointingHandCursor : Qt::ArrowCursor);

  auto details = u"%1 · Episode %2"_s.arg(content->isPlaying ? u"Watching"_s : u"Watched"_s)
                     .arg(content->progress);
  if (!content->episodeTitle.isEmpty()) details += u" · "_s + content->episodeTitle;
  detailsLabel_->setText(details);

  if (const auto iconName = statusIconName(content->updateState, content->isRecognized);
      iconName.isEmpty()) {
    iconLabel_->clear();
  } else {
    iconLabel_->setPixmap(theme()->getIcon(iconName).pixmap(QSize(16, 16)));
  }
  statusLabel_->setText(formatUpdateState(content->updateState));

  if (const auto color = statusColor(content->updateState); color.isValid()) {
    QPalette palette;  // only override text color, inherit the rest
    palette.setColor(QPalette::WindowText, color);
    statusLabel_->setPalette(palette);
    statusLabel_->setForegroundRole(QPalette::WindowText);
  } else {
    statusLabel_->setPalette({});
    statusLabel_->setForegroundRole(QPalette::PlaceholderText);
  }

  const auto [acceptText, cancelText] = formatUpdateActions(content->updateState);
  acceptButton_->setText(acceptText);
  acceptButton_->setVisible(!acceptText.isEmpty());
  cancelButton_->setText(cancelText);
  cancelButton_->setVisible(!cancelText.isEmpty());
  actions_->setVisible(!acceptText.isEmpty());
}

}  // namespace gui
