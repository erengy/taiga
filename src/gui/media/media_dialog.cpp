/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
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

#include <QMessageBox>
#include <QResizeEvent>

#include "gui/utils/format.hpp"
#include "gui/utils/image_provider.hpp"
#include "media/anime_season.hpp"
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

  ui_->splitter->setSizes({ui_->posterLabel->minimumWidth(), ui_->posterLabel->minimumWidth() * 4});

  ui_->verticalLayoutRewatching->setAlignment(Qt::AlignBottom);

  connect(&imageProvider, &ImageProvider::posterChanged, this, [this](int id) {
    if (id == m_anime.id) loadPosterImage();
  });

  connect(ui_->splitter, &QSplitter::splitterMoved, this, [this]() { resizePosterImage(); });

  connect(ui_->checkRewatching, &QCheckBox::stateChanged, this, [this](int state) {
    const bool isChecked = state == Qt::CheckState::Checked;
    const int progress = ui_->spinProgress->value();

    // Set status
    const int status =
        static_cast<int>(isChecked ? anime::list::Status::Watching : m_entry->status);
    if (const int index = ui_->comboStatus->findData(status); index > -1) {
      ui_->comboStatus->setCurrentIndex(index);
    }

    // Reset progress
    if (isChecked) {
      if (m_entry->status == anime::list::Status::Completed &&
          progress == m_entry->watched_episodes) {
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

  connect(ui_->checkDateStarted, &QCheckBox::stateChanged, this,
          [this](int state) { ui_->dateStarted->setEnabled(state == Qt::CheckState::Checked); });
  connect(ui_->checkDateCompleted, &QCheckBox::stateChanged, this,
          [this](int state) { ui_->dateCompleted->setEnabled(state == Qt::CheckState::Checked); });
}

void MediaDialog::resizeEvent(QResizeEvent* event) {
  QDialog::resizeEvent(event);
  resizePosterImage();
}

void MediaDialog::showEvent(QShowEvent* event) {
  QDialog::showEvent(event);
  resizePosterImage();
}

void MediaDialog::show(QWidget* parent, const Anime& anime, const std::optional<ListEntry> entry) {
  auto* dlg = new MediaDialog(parent);
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->setAnime(anime, entry);
  dlg->QDialog::show();
}

void MediaDialog::setAnime(const Anime& anime, const std::optional<ListEntry> entry) {
  m_anime = anime;
  m_entry = entry;

  loadPosterImage();
  initTitles();
  initDetails();
  initList();
}

void MediaDialog::initTitles() {
  const auto mainTitle = QString::fromStdString(m_anime.titles.romaji);
  setWindowTitle(mainTitle);
  ui_->titleLabel->setText(mainTitle);

  QList<QString> altTitles;
  const auto addTitle = [&mainTitle, &altTitles](const QString& title) {
    if (title.isEmpty() || title == mainTitle) return;
    altTitles.push_back(title);
  };
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

  const auto from_vector = [](const std::vector<std::string>& vector) {
    QStringList list;
    for (const auto& str : vector) {
      list.append(QString::fromStdString(str));
    }
    return list.join(", ");
  };

  auto seasonLabel = new QLabel(formatSeason(anime::Season(m_anime.date_started)), this);
  seasonLabel->setCursor(QCursor(Qt::CursorShape::WhatsThisCursor));
  seasonLabel->setToolTip(u"%1 to %2"_qs.arg(formatFuzzyDate(m_anime.date_started))
                              .arg(formatFuzzyDate(m_anime.date_finished)));

  if (!m_anime.titles.synonyms.empty()) {
    ui_->infoLayout->addRow(get_row_title(tr("Titles:")),
                            get_row_label(from_vector(m_anime.titles.synonyms)));
  }
  ui_->infoLayout->addRow(get_row_title(tr("Type:")), get_row_label(formatType(m_anime.type)));
  ui_->infoLayout->addRow(get_row_title(tr("Episodes:")),
                          get_row_label(QString::number(m_anime.episode_count)));
  if (m_anime.episode_length > 0 && m_anime.type != anime::Type::Tv) {
    const auto duration = formatEpisodeLength(m_anime.episode_length);
    const auto label = m_anime.episode_count == 1 ? duration : u"%1 per episode"_qs.arg(duration);
    ui_->infoLayout->addRow(get_row_title(tr("Duration:")), get_row_label(label));
  }
  ui_->infoLayout->addRow(get_row_title(tr("Status:")),
                          get_row_label(formatStatus(m_anime.status)));
  ui_->infoLayout->addRow(get_row_title(tr("Season:")), seasonLabel);
  ui_->infoLayout->addRow(get_row_title(tr("Score:")), get_row_label(formatScore(m_anime.score)));
  if (!m_anime.genres.empty()) {
    ui_->infoLayout->addRow(get_row_title(tr("Genres:")),
                            get_row_label(from_vector(m_anime.genres)));
  }
  if (!m_anime.tags.empty()) {
    ui_->infoLayout->addRow(get_row_title(tr("Tags:")), get_row_label(from_vector(m_anime.tags)));
  }
  if (!m_anime.studios.empty()) {
    ui_->infoLayout->addRow(get_row_title(tr("Studios:")),
                            get_row_label(from_vector(m_anime.studios)));
  }
  if (!m_anime.producers.empty()) {
    ui_->infoLayout->addRow(get_row_title(tr("Producers:")),
                            get_row_label(from_vector(m_anime.producers)));
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
  if (!ui_->comboScore->count()) {
    for (int i = 0; i <= 10; ++i) {
      ui_->comboScore->addItem(tr("%1").arg(i), i);
    }
  }
  ui_->comboScore->setCurrentIndex(m_entry->score / 10);

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

void MediaDialog::loadPosterImage() {
  const auto& posterPixmap = imageProvider.loadPoster(m_anime.id);
  ui_->posterLabel->setPixmap(posterPixmap);
  resizePosterImage();
}

void MediaDialog::resizePosterImage() {
  const auto& posterPixmap = ui_->posterLabel->pixmap();

  if (posterPixmap.isNull()) return;

  const int w = posterPixmap.width();
  const int h = posterPixmap.height();
  const int poster_w = ui_->posterLabel->width();
  const int height = h * (poster_w / static_cast<float>(w));

  ui_->posterLabel->setFixedHeight(height);
}

void MediaDialog::accept() {
  const int progress = ui_->spinProgress->value();
  const int rewatches = ui_->spinRewatches->value();
  const bool rewatching = ui_->checkRewatching->isChecked();
  const int status = ui_->comboStatus->currentData().toInt();
  const int score = ui_->comboScore->currentData().toInt();
  const auto date_started = ui_->checkDateStarted->isChecked()
                                ? std::optional<QDate>{ui_->dateStarted->date()}
                                : std::nullopt;
  const auto date_completed = ui_->checkDateCompleted->isChecked()
                                  ? std::optional<QDate>{ui_->dateCompleted->date()}
                                  : std::nullopt;
  const auto notes = ui_->plainTextEditNotes->toPlainText();

  // @TEMP
  const QList<QString> list{
      u"Progress: %1"_qs.arg(progress),
      u"Rewatches: %1"_qs.arg(rewatches),
      u"Rewatching: %1"_qs.arg(rewatching ? "Yes" : "No"),
      u"Status: %1"_qs.arg(formatListStatus(static_cast<anime::list::Status>(status))),
      u"Score: %1"_qs.arg(score),
      u"Date started: %1"_qs.arg(date_started ? formatDate(*date_started) : "-"),
      u"Date completed: %1"_qs.arg(date_completed ? formatDate(*date_completed) : "-"),
      u"Notes: %1"_qs.arg(notes),
  };
  QMessageBox::information(this, "Media", list.join("\n"));

  // @TODO: Add to queue

  QDialog::accept();
}

}  // namespace gui
