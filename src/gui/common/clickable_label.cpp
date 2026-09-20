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

#include "clickable_label.hpp"

#include <QHelpEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QToolTip>

namespace gui {

ClickableLabel::ClickableLabel(QWidget* parent, Qt::WindowFlags f) : QLabel(parent, f) {}

void ClickableLabel::setElidable(const bool elidable) {
  m_elidable = elidable;

  // The text is allowed to be wider than the space we are given.
  setSizePolicy(elidable ? QSizePolicy::Ignored : QSizePolicy::Preferred, QSizePolicy::Preferred);

  update();
}

bool ClickableLabel::event(QEvent* event) {
  if (event->type() == QEvent::ToolTip && toolTip().isEmpty()) {
    if (m_elidable && elidedText() != text()) {
      QToolTip::showText(static_cast<QHelpEvent*>(event)->globalPos(), text(), this);
      return true;
    }
  }

  return QLabel::event(event);
}

void ClickableLabel::mousePressEvent(QMouseEvent* event) {
  emit clicked(event->button());

  QLabel::mousePressEvent(event);
}

void ClickableLabel::paintEvent(QPaintEvent* event) {
  if (!m_elidable) {
    QLabel::paintEvent(event);
    return;
  }

  QPainter painter(this);
  style()->drawItemText(&painter, contentsRect(), alignment(), palette(), isEnabled(), elidedText(),
                        foregroundRole());
}

QString ClickableLabel::elidedText() const {
  return fontMetrics().elidedText(text(), Qt::ElideRight, contentsRect().width());
}

}  // namespace gui
