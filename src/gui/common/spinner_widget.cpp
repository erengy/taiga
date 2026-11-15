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
  setSize(size);
  hide();

  connect(&timer_, &QTimer::timeout, this, &SpinnerWidget::advance);
}

void SpinnerWidget::setSize(int size) {
  setFixedSize(size, size);

  basePixmap_ = theme()->getIcon("progress_activity").pixmap(QSize(size, size));
  setPixmap(basePixmap_);
}

void SpinnerWidget::start() {
  show();

  if (timer_.isActive()) return;

  timer_.start(kSpinnerIntervalMs);
}

void SpinnerWidget::stop() {
  timer_.stop();
  angle_ = 0.0;
  setPixmap(basePixmap_);
  hide();
}

void SpinnerWidget::advance() {
  angle_ += kSpinnerDegreesPerTick;
  if (angle_ >= 360.0) angle_ -= 360.0;

  const qreal dpr = basePixmap_.devicePixelRatio();

  QPixmap frame(basePixmap_.size());
  frame.setDevicePixelRatio(dpr);
  frame.fill(Qt::transparent);

  const QPointF center(basePixmap_.width() / dpr / 2.0, basePixmap_.height() / dpr / 2.0);

  QPainter painter(&frame);
  paintSpinner(&painter, basePixmap_, center, angle_);

  setPixmap(frame);
}

}  // namespace gui
