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

void MediaDialog::show(QWidget* parent, const Anime& anime_item) {
  auto* dlg = new MediaDialog(parent);
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->setAnime(anime_item);
  dlg->QDialog::show();
}

void MediaDialog::setAnime(const Anime& anime_item) {
  m_anime = anime_item;

  const auto title = QString::fromStdString(m_anime.titles.romaji);
  setWindowTitle(title);
  ui_->titleLabel->setText(title);

  loadPosterImage();
  initDetails();
}

void MediaDialog::initDetails() const {
  while (ui_->infoLayout->rowCount() > 0) {
    ui_->infoLayout->removeRow(0);
  }

  const auto get_row_title = [](const QString& text) {
    auto* label = new QLabel(text);
    label->setAlignment(Qt::AlignRight | Qt::AlignTop);
    label->setFont([label]() {
      auto font = label->font();
      font.setWeight(QFont::Weight::DemiBold);
      return font;
    }());
    return label;
  };

  const auto get_row_label = [](const QString& text) {
    auto* label = new QLabel(text);
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

  auto episodesLabel = new QLabel(tr("%1").arg(m_anime.episode_count));
  if (m_anime.episode_length) {
    episodesLabel->setCursor(QCursor(Qt::CursorShape::WhatsThisCursor));
    episodesLabel->setToolTip(u"%1 minutes per episode"_qs.arg(m_anime.episode_length));
  }

  auto seasonLabel = new QLabel(QString::fromStdString(m_anime.start_date));
  seasonLabel->setCursor(QCursor(Qt::CursorShape::WhatsThisCursor));
  seasonLabel->setToolTip(u"%1 to %1"_qs.arg(QString::fromStdString(m_anime.start_date))
                              .arg(QString::fromStdString(m_anime.end_date)));

  if (!m_anime.titles.synonyms.empty()) {
    ui_->infoLayout->addRow(get_row_title(tr("Synonyms:")),
                            get_row_label(from_vector(m_anime.titles.synonyms)));
  }
  ui_->infoLayout->addRow(get_row_title(tr("Type:")),
                          get_row_label(u"%1"_qs.arg(static_cast<int>(m_anime.type))));
  ui_->infoLayout->addRow(get_row_title(tr("Episodes:")), episodesLabel);
  ui_->infoLayout->addRow(get_row_title(tr("Status:")),
                          get_row_label(u"%1"_qs.arg(static_cast<int>(m_anime.status))));
  ui_->infoLayout->addRow(get_row_title(tr("Season:")), seasonLabel);
  ui_->infoLayout->addRow(get_row_title(tr("Score:")), get_row_label(u"%1"_qs.arg(m_anime.score)));
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

void MediaDialog::loadPosterImage() {
  QImageReader reader("./data/poster.jpg");
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
