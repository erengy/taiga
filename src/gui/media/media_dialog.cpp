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

#include "media_dialog.hpp"

#include <QDesktopServices>
#include <QResizeEvent>
#include <QUrl>
#include <algorithm>

#include "base/string.hpp"
#include "gui/utils/format.hpp"
#include "gui/utils/image_provider.hpp"
#include "gui/utils/rating.hpp"
#include "gui/utils/theme.hpp"
#include "gui/utils/widgets.hpp"
#include "media/anime_db.hpp"
#include "media/anime_list_utils.hpp"
#include "media/anime_season.hpp"
#include "media/anime_utils.hpp"
#include "sync/service.hpp"
#include "taiga/session.hpp"
#include "track/recognition_cache.hpp"
#include "ui_media_dialog.h"

#ifdef Q_OS_WINDOWS
#include "gui/platforms/windows.hpp"
#endif

namespace gui {

MediaDialog::MediaDialog(QWidget* parent) : QDialog(parent), ui_(new Ui::MediaDialog) {
  ui_->setupUi(this);

#ifdef Q_OS_WINDOWS
  enableMicaBackground(this);
#endif

  if (const auto geometry = taiga::session.mediaDialogGeometry(); !geometry.isEmpty()) {
    restoreGeometry(geometry);
    centerWidgetToScreen(this);
  }

  ui_->posterLabel->setFrameShape(QFrame::Shape::NoFrame);

  ui_->splitter->setSizes({ui_->posterLabel->minimumWidth(), ui_->posterLabel->minimumWidth() * 4});
  if (const auto state = taiga::session.mediaDialogSplitterState(); !state.isEmpty()) {
    ui_->splitter->restoreState(state);
  }

  ui_->tabWidget->setTabIcon(static_cast<int>(MediaDialogPage::Details), theme.getIcon("info"));
  ui_->tabWidget->setTabIcon(static_cast<int>(MediaDialogPage::List), theme.getIcon("list_alt"));
  ui_->tabWidget->setTabIcon(static_cast<int>(MediaDialogPage::Settings),
                             theme.getIcon("settings"));

  ui_->verticalLayoutRewatching->setAlignment(Qt::AlignBottom);

  connect(&imageProvider, &ImageProvider::posterChanged, this, [this](int id) {
    if (id == m_anime.id) loadPosterImage();
  });

  connect(&anime::db, &anime::Database::itemUpdated, this, [this](const int id) {
    if (id != m_anime.id) return;
    m_anime = *anime::db.item(id);
    initTitles();
    initDetails();
    initSettings();
  });

  connect(&anime::db, &anime::Database::entryUpdated, this, [this](const int id) {
    if (id != m_anime.id) return;
    initList();
  });

  connect(&anime::db, &anime::Database::entryDeleted, this, [this](const int id) {
    if (id != m_anime.id) return;
    initList();
  });

  connect(ui_->posterLabel, &ClickableLabel::clicked, this, [this](Qt::MouseButton button) {
    if (button == Qt::MouseButton::LeftButton) {
      QUrl url{sync::animePageUrl(m_anime.id)};
      QDesktopServices::openUrl(url);
    }
  });

  connect(ui_->splitter, &QSplitter::splitterMoved, this, [this]() { resizePosterImage(); });

  connect(ui_->checkRewatching, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
    const bool isChecked = state == Qt::CheckState::Checked;
    const int progress = ui_->spinProgress->value();

    // Set status
    switch (sync::currentServiceId()) {
      case sync::ServiceId::MyAnimeList:
        // MyAnimeList tracks rewatching independently of status.
        break;
      case sync::ServiceId::Kitsu:
      case sync::ServiceId::AniList: {
        const int status =
            static_cast<int>(isChecked ? anime::list::Status::Watching : m_entry->status);
        if (const int index = ui_->comboStatus->findData(status); index > -1) {
          ui_->comboStatus->setCurrentIndex(index);
        }
        break;
      }
    }

    // Reset progress
    if (isChecked) {
      const bool isCompleted = m_entry->status == anime::list::Status::Completed;
      if (isCompleted && progress == m_entry->watched_episodes) {
        ui_->spinProgress->setValue(0);
      }
    } else {
      if (progress == 0) {
        ui_->spinProgress->setValue(m_entry->watched_episodes);
      }
    }
  });

  connect(ui_->comboStatus, &QComboBox::currentIndexChanged, this, [this](int index) {
    const int status = ui_->comboStatus->itemData(index).toInt();
    if (status != static_cast<int>(anime::list::Status::Completed)) return;
    if (m_entry->status == anime::list::Status::Completed) return;
    if (m_anime.episode_count < 1) return;
    ui_->spinProgress->setValue(m_anime.episode_count);
  });

  connect(ui_->checkDateStarted, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
    ui_->dateStarted->setEnabled(state == Qt::CheckState::Checked);
  });
  connect(ui_->checkDateCompleted, &QCheckBox::checkStateChanged, this,
          [this](Qt::CheckState state) {
            ui_->dateCompleted->setEnabled(state == Qt::CheckState::Checked);
          });
}

void MediaDialog::closeEvent(QCloseEvent* event) {
  taiga::session.setMediaDialogGeometry(saveGeometry());
  taiga::session.setMediaDialogSplitterState(ui_->splitter->saveState());
  event->accept();
}

void MediaDialog::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_F5) {
    imageProvider.fetchPoster(m_anime.id);
    sync::fetchAnime(m_anime.id);
    return;
  }

  QDialog::keyPressEvent(event);
}

void MediaDialog::resizeEvent(QResizeEvent* event) {
  QDialog::resizeEvent(event);
  resizePosterImage();
}

void MediaDialog::showEvent(QShowEvent* event) {
  QDialog::showEvent(event);
  resizePosterImage();
}

void MediaDialog::show(QWidget* parent, MediaDialogPage page, const Anime& anime) {
  auto* dlg = new MediaDialog(parent);
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->setAnime(anime);
  dlg->ui_->tabWidget->setCurrentIndex(static_cast<int>(page));
  dlg->QDialog::show();
}

void MediaDialog::setAnime(const Anime& anime) {
  m_anime = anime;

  loadPosterImage();
  initTitles();
  initDetails();
  initList();
  initSettings();

  if (anime::isStale(anime)) {
    sync::fetchAnime(anime.id);
  }
}

void MediaDialog::initTitles() {
  const auto mainTitle = QString::fromStdString(anime::preferredTitle(m_anime));
  setWindowTitle(mainTitle);
  ui_->titleLabel->setText(mainTitle);

  QList<QString> altTitles;
  const auto addTitle = [&mainTitle, &altTitles](const QString& title) {
    if (title.isEmpty() || title == mainTitle) return;
    altTitles.push_back(title);
  };
  addTitle(QString::fromStdString(m_anime.titles.romaji));
  addTitle(QString::fromStdString(m_anime.titles.english));
  addTitle(QString::fromStdString(m_anime.titles.japanese));
  if (!altTitles.isEmpty()) ui_->altTitlesLabel->setText(altTitles.join(", "));
  ui_->altTitlesLabel->setHidden(altTitles.isEmpty());
}

void MediaDialog::initDetails() {
  while (ui_->infoLayout->rowCount() > 0) {
    ui_->infoLayout->removeRow(0);
  }

  const auto get_row_title = [this](const QString& text) {
    auto* label = new QLabel(text, this);
    label->setAlignment(Qt::AlignRight | Qt::AlignTop);
    label->setFont([label]() {
      auto font = label->font();
      font.setWeight(QFont::Weight::DemiBold);
      return font;
    }());
    return label;
  };

  const auto get_row_label = [this](const QString& text) {
    auto* label = new QLabel(text, this);
    label->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
    label->setTextInteractionFlags(Qt::TextInteractionFlag::TextSelectableByMouse);
    label->setOpenExternalLinks(true);
    label->setWordWrap(true);
    return label;
  };

  auto seasonLabel = new QLabel(formatSeason(anime::Season(m_anime.date_started)), this);
  seasonLabel->setCursor(QCursor(Qt::CursorShape::WhatsThisCursor));
  seasonLabel->setToolTip(formatFuzzyDateRange(m_anime.date_started, m_anime.date_finished));

  if (!m_anime.titles.synonyms.empty()) {
    ui_->infoLayout->addRow(get_row_title(tr("Titles:")),
                            get_row_label(joinStrings(m_anime.titles.synonyms)));
  }
  ui_->infoLayout->addRow(get_row_title(tr("Type:")), get_row_label(formatType(m_anime.type)));
  ui_->infoLayout->addRow(get_row_title(tr("Episodes:")),
                          get_row_label(formatNumber(m_anime.episode_count, "?")));
  if (m_anime.episode_length > 0 && m_anime.type != anime::Type::Tv) {
    const auto duration = formatEpisodeLength(m_anime.episode_length);
    const auto label = m_anime.episode_count == 1 ? duration : u"%1 per episode"_s.arg(duration);
    ui_->infoLayout->addRow(get_row_title(tr("Duration:")), get_row_label(label));
  }
  ui_->infoLayout->addRow(get_row_title(tr("Status:")),
                          get_row_label(formatStatus(m_anime.status)));
  ui_->infoLayout->addRow(get_row_title(tr("Season:")), seasonLabel);
  ui_->infoLayout->addRow(get_row_title(tr("Score:")), get_row_label(formatScore(m_anime.score)));
  if (!m_anime.genres.empty()) {
    ui_->infoLayout->addRow(get_row_title(tr("Genres:")),
                            get_row_label(joinStrings(m_anime.genres)));
  }
  if (!m_anime.tags.empty()) {
    ui_->infoLayout->addRow(get_row_title(tr("Tags:")), get_row_label(joinStrings(m_anime.tags)));
  }
  if (!m_anime.studios.empty()) {
    ui_->infoLayout->addRow(get_row_title(tr("Studios:")),
                            get_row_label(joinStrings(m_anime.studios)));
  }
  if (!m_anime.producers.empty()) {
    ui_->infoLayout->addRow(get_row_title(tr("Producers:")),
                            get_row_label(joinStrings(m_anime.producers)));
  }

  const auto synopsis = QString::fromStdString(m_anime.synopsis);

  ui_->synopsisHeader->setHidden(synopsis.isEmpty());

  ui_->synopsis->document()->setDocumentMargin(0);
  ui_->synopsis->viewport()->setAutoFillBackground(false);
  ui_->synopsis->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
  ui_->synopsis->setHtml(synopsis);
  ui_->synopsis->setHidden(synopsis.isEmpty());
}

void MediaDialog::initList() {
  const auto entry = anime::db.entry(m_anime.id);
  m_entry = entry ? std::optional<ListEntry>{*entry} : std::nullopt;

  ui_->tabWidget->setTabVisible(1, m_entry.has_value());

  if (!m_entry.has_value()) return;

  // Episodes watched
  if (m_anime.episode_count > 0) {
    ui_->spinProgress->setMaximum(m_anime.episode_count);
  }
  ui_->spinProgress->setValue(m_entry->watched_episodes);

  // Times rewatched
  ui_->spinRewatches->setValue(m_entry->rewatched_times);

  // Rewatching
  ui_->checkRewatching->setChecked(m_entry->rewatching);

  // Status
  ui_->comboStatus->clear();
  for (const auto status : anime::list::kStatuses) {
    ui_->comboStatus->addItem(formatListStatus(status), static_cast<int>(status));
    if (status == m_entry->status) {
      ui_->comboStatus->setCurrentIndex(ui_->comboStatus->count() - 1);
    }
  }

  // Score
  const bool useScoreSpinBox = usesRatingSpinBox();
  ui_->comboScore->setVisible(!useScoreSpinBox);
  ui_->spinScore->setVisible(useScoreSpinBox);
  if (useScoreSpinBox) {
    populateRatingSpinBox(ui_->spinScore);
    setRatingSpinBoxValue(ui_->spinScore, m_entry->score);
  } else {
    populateRatingComboBox(ui_->comboScore);
    setRatingComboBoxValue(ui_->comboScore, m_entry->score);
  }

  const auto fuzzy_to_date = [](const FuzzyDate& date) {
    return QDate{date.year(), date.month(), date.day()};
  };

  // Date started & completed
  ui_->checkDateStarted->setChecked(static_cast<bool>(m_entry->date_started));
  ui_->checkDateCompleted->setChecked(static_cast<bool>(m_entry->date_completed));
  if (m_anime.date_started) {
    ui_->dateStarted->setMinimumDate(fuzzy_to_date(m_anime.date_started));
    ui_->dateCompleted->setMinimumDate(fuzzy_to_date(m_anime.date_started));
  }
  ui_->dateStarted->setDate(m_entry->date_started ? fuzzy_to_date(m_entry->date_started)
                                                  : QDate::currentDate());
  ui_->dateCompleted->setDate(m_entry->date_completed ? fuzzy_to_date(m_entry->date_completed)
                                                      : QDate::currentDate());

  // Notes
  ui_->plainTextEditNotes->setPlainText(QString::fromStdString(m_entry->notes));
}

void MediaDialog::initSettings() {
  const auto settings = anime::db.settings(m_anime.id);
  m_settings = settings ? *settings : anime::Settings{.id = m_anime.id};

  ui_->comboDisplayTitle->clear();

  QList<QString> candidates;
  const auto addCandidate = [&candidates](const QString& title) {
    if (!title.isEmpty() && !candidates.contains(title)) {
      candidates.push_back(title);
    }
  };
  addCandidate(QString::fromStdString(m_anime.titles.romaji));
  addCandidate(QString::fromStdString(m_anime.titles.english));
  addCandidate(QString::fromStdString(m_anime.titles.japanese));
  for (const auto& synonym : m_anime.titles.synonyms) {
    addCandidate(QString::fromStdString(synonym));
  }
  addCandidate(QString::fromStdString(m_settings.display_title));
  ui_->comboDisplayTitle->addItems(candidates);

  ui_->comboDisplayTitle->setCurrentText(QString::fromStdString(m_settings.display_title));
  ui_->comboDisplayTitle->lineEdit()->setPlaceholderText(
      QString::fromStdString(anime::preferredTitle(m_anime)));

  QStringList synonyms;
  for (const auto& synonym : m_settings.synonyms) {
    synonyms.push_back(QString::fromStdString(synonym));
  }
  ui_->recognitionTitles->setPlainText(synonyms.join("\n"));
}

void MediaDialog::loadPosterImage() {
  const auto posterPixmap = imageProvider.loadPoster(m_anime.id);
  ui_->posterLabel->setPixmap(posterPixmap);
  resizePosterImage();
}

void MediaDialog::resizePosterImage() {
  const auto& posterPixmap = ui_->posterLabel->pixmap();
  const int label_w = ui_->posterLabel->width();

  if (posterPixmap.isNull()) {
    ui_->posterLabel->setFixedHeight(label_w * 3 / 2);
    return;
  }

  const int w = posterPixmap.width();
  const int h = posterPixmap.height();
  const int height = h * (label_w / static_cast<float>(w));

  ui_->posterLabel->setFixedHeight(height);
}

void MediaDialog::accept() {
  if (m_entry) {
    m_entry->watched_episodes = ui_->spinProgress->value();
    m_entry->rewatched_times = ui_->spinRewatches->value();
    m_entry->rewatching = ui_->checkRewatching->isChecked();
    m_entry->status = ui_->comboStatus->currentData().value<anime::list::Status>();
    m_entry->score = usesRatingSpinBox() ? ratingSpinBoxValue(ui_->spinScore)
                                         : ui_->comboScore->currentData().toInt();
    m_entry->date_started = ui_->checkDateStarted->isChecked()
                                ? FuzzyDate{ui_->dateStarted->date().toStdSysDays()}
                                : FuzzyDate{};
    m_entry->date_completed = ui_->checkDateCompleted->isChecked()
                                  ? FuzzyDate{ui_->dateCompleted->date().toStdSysDays()}
                                  : FuzzyDate{};
    m_entry->notes = ui_->plainTextEditNotes->toPlainText().toStdString();

    anime::list::save(*m_entry);
  }

  m_settings.display_title = ui_->comboDisplayTitle->currentText().trimmed().toStdString();

  m_settings.synonyms.clear();
  for (const auto& line : ui_->recognitionTitles->toPlainText().split("\n")) {
    const auto synonym = line.trimmed().toStdString();
    if (synonym.empty()) continue;
    if (!std::ranges::contains(m_settings.synonyms, synonym)) {
      m_settings.synonyms.push_back(synonym);
    }
  }

  anime::db.updateSettings(m_settings);

  if (const auto* anime = anime::db.item(m_settings.id)) {
    track::recognition::cache()->update(*anime);
  }

  QDialog::accept();
}

}  // namespace gui
