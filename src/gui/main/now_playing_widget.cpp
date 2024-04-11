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

#include "now_playing_widget.hpp"

#include <QBoxLayout>
#include <QLabel>

#include "gui/utils/theme.hpp"

namespace gui {

NowPlayingWidget::NowPlayingWidget(QWidget* parent) : QFrame(parent) {
  setObjectName("nowPlaying");
  setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Maximum);

  const auto layout = new QHBoxLayout(this);
  layout->setContentsMargins(16, 16, 16, 16);
  setLayout(layout);

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

  // Timer
  m_timerLabel = new QLabel(this);
  m_timerLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  layout->addWidget(m_timerLabel);

  refresh();
}

void NowPlayingWidget::refresh() {
  m_iconLabel->setToolTip(
      "<b>Media player:</b> mpv<br>"
      "<b>Episode title:</b> Tiger and Dragon<br>"
      "<b>Group:</b> TaigaSubs<br>"
      "<b>Video:</b> 1080p");

  // @TODO: Go to media details on click
  m_mainLabel->setText(u"Watching <a href=\"#\" style=\"%3\">%1</a> – Episode %2"_qs
                           .arg("Toradora!")
                           .arg("1/25")
                           .arg("color: inherit; font-weight: 600; text-decoration: none;"));

  m_timerLabel->setText("List update in 00:00");
}

}  // namespace gui
