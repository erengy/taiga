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

#include "widgets.hpp"

#include <QGuiApplication>
#include <QMessageBox>
#include <QScreen>

namespace gui {

void centerWidgetToScreen(QWidget* widget) {
  if (!widget) return;
  const auto screen = QGuiApplication::screenAt(widget->frameGeometry().topLeft());
  if (!screen) return;
  widget->move(screen->availableGeometry().center() - QRect{{}, widget->frameSize()}.center());
};

bool confirm(QWidget* parent, const QString& text, const QString& informativeText,
             const QString& confirmButtonText) {
  QMessageBox msgBox(parent);
  msgBox.setIcon(QMessageBox::Icon::Question);
  msgBox.setText(text);
  msgBox.setInformativeText(informativeText);

  const auto confirmButton =
      msgBox.addButton(confirmButtonText, QMessageBox::ButtonRole::DestructiveRole);
  msgBox.addButton(QMessageBox::Cancel);
  msgBox.setDefaultButton(QMessageBox::Cancel);

  msgBox.exec();

  return msgBox.clickedButton() == reinterpret_cast<QAbstractButton*>(confirmButton);
}

}  // namespace gui
