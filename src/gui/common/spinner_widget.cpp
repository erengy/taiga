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

#include "spinner_widget.hpp"

#include <QPainter>

#include "gui/utils/painters.hpp"
#include "gui/utils/theme.hpp"

namespace gui {

SpinnerWidget::SpinnerWidget(QWidget* parent, int size) : QLabel(parent) {
  setFixedSize(size, size);

  m_basePixmap = theme.getIcon("progress_activity").pixmap(QSize(size, size));
  setPixmap(m_basePixmap);
  hide();

  connect(&m_timer, &QTimer::timeout, this, &SpinnerWidget::advance);
}

void SpinnerWidget::start() {
  show();

  if (m_timer.isActive()) return;

  m_timer.start(kSpinnerIntervalMs);
}

void SpinnerWidget::stop() {
  m_timer.stop();
  m_angle = 0.0;
  setPixmap(m_basePixmap);
  hide();
}

void SpinnerWidget::advance() {
  m_angle += kSpinnerDegreesPerTick;
  if (m_angle >= 360.0) m_angle -= 360.0;

  const qreal dpr = m_basePixmap.devicePixelRatio();

  QPixmap frame(m_basePixmap.size());
  frame.setDevicePixelRatio(dpr);
  frame.fill(Qt::transparent);

  const QPointF center(m_basePixmap.width() / dpr / 2.0, m_basePixmap.height() / dpr / 2.0);

  QPainter painter(&frame);
  paintSpinner(&painter, m_basePixmap, center, m_angle);

  setPixmap(frame);
}

}  // namespace gui
