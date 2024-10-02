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

#include <QImageReader>
#include <QResizeEvent>

#include "gui/utils/format.hpp"
#include "media/anime_season.hpp"
#include "ui_media_dialog.h"

namespace gui {

MediaDialog::MediaDialog(QWidget* parent) : QDialog(parent), ui_(new Ui::MediaDialog) {
  ui_->setupUi(this);

  ui_->titleLabel->setStyleSheet("font-size: 20px; font-weight: 600;");

  connect(ui_->splitter, &QSplitter::splitterMoved, this, [this]() { resizePosterImage(); });

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

  const auto title = QString::fromStdString(m_anime.titles.romaji);
  setWindowTitle(title);
  ui_->titleLabel->setText(title);

  loadPosterImage();
  initDetails();
  initList();
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

  auto episodesLabel = new QLabel(tr("%1").arg(m_anime.episode_count), this);
  if (m_anime.episode_length) {
    episodesLabel->setCursor(QCursor(Qt::CursorShape::WhatsThisCursor));
    episodesLabel->setToolTip(u"%1 minutes per episode"_qs.arg(m_anime.episode_length));
  }

  auto seasonLabel = new QLabel(fromSeason(anime::Season(m_anime.start_date)), this);
  seasonLabel->setCursor(QCursor(Qt::CursorShape::WhatsThisCursor));
  seasonLabel->setToolTip(
      u"%1 to %2"_qs.arg(fromFuzzyDate(m_anime.start_date)).arg(fromFuzzyDate(m_anime.end_date)));

  if (!m_anime.titles.synonyms.empty()) {
    ui_->infoLayout->addRow(get_row_title(tr("Synonyms:")),
                            get_row_label(from_vector(m_anime.titles.synonyms)));
  }
  ui_->infoLayout->addRow(get_row_title(tr("Type:")), get_row_label(fromType(m_anime.type)));
  ui_->infoLayout->addRow(get_row_title(tr("Episodes:")), episodesLabel);
  ui_->infoLayout->addRow(get_row_title(tr("Status:")), get_row_label(fromStatus(m_anime.status)));
  ui_->infoLayout->addRow(get_row_title(tr("Season:")), seasonLabel);
  ui_->infoLayout->addRow(get_row_title(tr("Score:")), get_row_label(formatScore(m_anime.score)));
  if (!m_anime.genres.empty()) {
    ui_->infoLayout->addRow(get_row_title(tr("Genres:")),
                            get_row_label(from_vector(m_anime.genres)));
  }
  if (!m_anime.studios.empty()) {
    ui_->infoLayout->addRow(get_row_title(tr("Studios:")),
                            get_row_label(from_vector(m_anime.studios)));
  }
  if (!m_anime.producers.empty()) {
    ui_->infoLayout->addRow(get_row_title(tr("Producers:")),
                            get_row_label(from_vector(m_anime.producers)));
  }

  ui_->synopsisHeader->setStyleSheet("font-weight: 600;");

  ui_->synopsis->document()->setDocumentMargin(0);
  ui_->synopsis->viewport()->setAutoFillBackground(false);
  ui_->synopsis->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
  ui_->synopsis->setHtml(QString::fromStdString(m_anime.synopsis));
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
    ui_->comboStatus->addItem(fromListStatus(status), static_cast<int>(status));
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

  // Date started
  ui_->checkDateStarted->setChecked((bool)m_entry->date_start);
  if (m_anime.start_date) {
    ui_->dateStarted->setMinimumDate(fuzzy_to_date(m_anime.start_date));
  }
  if (m_entry->date_start) {
    ui_->dateStarted->setDate(fuzzy_to_date(m_entry->date_start));
  } else if (m_anime.start_date) {
    ui_->dateStarted->setDate(fuzzy_to_date(m_anime.start_date));
  }

  // Date completed
  ui_->checkDateCompleted->setChecked((bool)m_entry->date_finish);
  if (m_anime.start_date) {
    ui_->dateCompleted->setMinimumDate(fuzzy_to_date(m_anime.start_date));
  }
  if (m_entry->date_finish) {
    ui_->dateCompleted->setDate(fuzzy_to_date(m_entry->date_finish));
  } else if (m_anime.end_date) {
    ui_->dateCompleted->setDate(fuzzy_to_date(m_anime.end_date));
  }

  // Notes
  ui_->plainTextEditNotes->setPlainText(QString::fromStdString(m_entry->notes));
}

void MediaDialog::loadPosterImage() {
  QImageReader reader(u"./data/cache/image/%1.jpg"_qs.arg(m_anime.id));
  const QImage image = reader.read();
  if (!image.isNull()) {
    m_pixmap = QPixmap::fromImage(image);
    ui_->posterLabel->setPixmap(m_pixmap);
    resizePosterImage();
  }
}

void MediaDialog::resizePosterImage() {
  const int w = m_pixmap.width();
  const int h = m_pixmap.height();
  const auto poster_w = ui_->posterLabel->width();
  const int height = h * (poster_w / static_cast<float>(w));
  ui_->posterLabel->setFixedHeight(height);
}

}  // namespace gui
