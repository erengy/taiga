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

#include "status_bar.hpp"

#include <QLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QStyleOption>
#include <algorithm>

namespace gui {

namespace {

// `QStatusBarPrivate::messageRect` hardcodes these, so `setContentsMargins` or
// stylesheet properties have no effect.
constexpr int kMessageLeftMargin = 12;
constexpr int kMessageRightMargin = 12;

}  // namespace

StatusBar::StatusBar(QWidget* parent) : QStatusBar(parent) {}

QRect StatusBar::messageRect() const {
  int right = width() - kMessageRightMargin;

  if (const auto* box = layout()) {
    for (int i = 0; i < box->count(); ++i) {
      if (const auto* widget = box->itemAt(i)->widget(); widget && widget->isVisible()) {
        right = std::min(right, widget->x() - 2);
      }
    }
  }

  return QRect(kMessageLeftMargin, 0, right - kMessageLeftMargin, height());
}

void StatusBar::paintEvent(QPaintEvent* event) {
  QPainter painter(this);

  QStyleOption panelOpt;
  panelOpt.initFrom(this);
  style()->drawPrimitive(QStyle::PE_PanelStatusBar, &panelOpt, &painter, this);

  const bool haveMessage = !currentMessage().isEmpty();

  if (const auto* box = layout()) {
    for (int i = 0; i < box->count(); ++i) {
      const auto* widget = box->itemAt(i)->widget();
      if (!widget || !widget->isVisible()) continue;

      const QRect itemRect = widget->geometry().adjusted(-2, -1, 2, 1);
      if (!event->rect().intersects(itemRect)) continue;

      QStyleOption itemOpt;
      itemOpt.rect = itemRect;
      itemOpt.palette = palette();
      itemOpt.state = QStyle::State_None;
      style()->drawPrimitive(QStyle::PE_FrameStatusBarItem, &itemOpt, &painter, widget);
    }
  }

  if (haveMessage) {
    painter.setPen(palette().windowText().color());
    painter.drawText(messageRect(), Qt::AlignLeading | Qt::AlignVCenter | Qt::TextSingleLine,
                     currentMessage());
  }
}

}  // namespace gui
