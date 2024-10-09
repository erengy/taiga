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

#include "combobox.hpp"

#include <QKeyEvent>
#include <QMouseEvent>

namespace gui {

ComboBox::ComboBox(QWidget* parent) : QComboBox(parent) {}

void ComboBox::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key::Key_Escape) {
    this->setCurrentIndex(-1);
    return;
  }

  QComboBox::keyPressEvent(event);
}

void ComboBox::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::MouseButton::MiddleButton ||
      event->button() == Qt::MouseButton::RightButton) {
    this->setCurrentIndex(-1);
    return;
  }

  QComboBox::mousePressEvent(event);
}

}  // namespace gui
