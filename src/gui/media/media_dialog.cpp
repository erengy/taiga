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

  setWindowTitle(QString::fromStdString(m_anime.title));
  ui_->titleLabel->setText(QString::fromStdString(m_anime.title));

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
    label->setWordWrap(true);
    return label;
  };

  auto episodesLabel = new QLabel(tr("%1").arg(m_anime.episode_count));
  episodesLabel->setCursor(QCursor(Qt::CursorShape::WhatsThisCursor));
  episodesLabel->setToolTip("24 minutes per episode");

  auto seasonLabel = new QLabel(QString::fromStdString(m_anime.season));
  seasonLabel->setCursor(QCursor(Qt::CursorShape::WhatsThisCursor));
  seasonLabel->setToolTip("Oct 2, 2008 to Mar 26, 2009");

  auto producersLabel = new QLabel(
      "<a href=\"https://anilist.co/anime/4224/Toradora\">Genco</a>, Starchild "
      "Records, Magic Capsule, NIS America Inc., Yomiuri Advertising, TV Tokyo "
      "Music, Hanabee Entertainment");
  producersLabel->setOpenExternalLinks(true);
  producersLabel->setWordWrap(true);

  ui_->infoLayout->addRow(get_row_title(tr("Synonyms:")),
                          get_row_label(u"Tiger X Dragon, 龙与虎, Τίγρης και Δράκος"_qs));
  ui_->infoLayout->addRow(get_row_title(tr("Type:")), get_row_label("TV"));
  ui_->infoLayout->addRow(get_row_title(tr("Episodes:")), episodesLabel);
  ui_->infoLayout->addRow(get_row_title(tr("Status:")), get_row_label("Finished airing"));
  ui_->infoLayout->addRow(get_row_title(tr("Season:")), seasonLabel);
  ui_->infoLayout->addRow(get_row_title(tr("Score:")), get_row_label("78%"));
  ui_->infoLayout->addRow(get_row_title(tr("Genres:")),
                          get_row_label("Comedy, Drama, Romance, Slice of Life"));
  ui_->infoLayout->addRow(get_row_title(tr("Studios:")), get_row_label("J.C. Staff"));
  ui_->infoLayout->addRow(get_row_title(tr("Producers:")), producersLabel);

  const QString synopsis =
      "Ryuuji Takasu is a gentle high school student with a love for "
      "housework; but in contrast to his kind nature, he has an intimidating "
      "face that often gets him labeled as a delinquent. On the other hand is "
      "Taiga Aisaka, a small, doll-like student, who is anything but a cute "
      "and fragile girl. Equipped with a wooden katana and feisty personality, "
      "Taiga is known throughout the school as the \"Palmtop "
      "Tiger.\"<br><br>One day, an embarrassing mistake causes the two "
      "students to cross paths. Ryuuji discovers that Taiga actually has a "
      "sweet side: she has a crush on the popular vice president, Yuusaku "
      "Kitamura, who happens to be his best friend. But things only get "
      "crazier when Ryuuji reveals that he has a crush on Minori "
      "Kushieda--Taiga's best friend!<br><br><i>Toradora!</i> is a romantic "
      "comedy that follows this odd duo as they embark on a quest to help each "
      "other with their respective crushes, forming an unlikely alliance in "
      "the process.";

  ui_->synopsisHeader->setStyleSheet("font-weight: 600;");

  ui_->synopsis->document()->setDocumentMargin(0);
  ui_->synopsis->viewport()->setAutoFillBackground(false);
  ui_->synopsis->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
  ui_->synopsis->setHtml(synopsis);
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
