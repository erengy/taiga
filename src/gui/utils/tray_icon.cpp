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

#include "tray_icon.hpp"

#include <QMenu>
#include <QPainter>
#include <QSystemTrayIcon>
#include <utility>

#include "gui/utils/theme.hpp"

namespace gui {

TrayIcon::TrayIcon(QObject* parent, QIcon icon, QMenu* menu) : baseIcon_(std::move(icon)) {
  if (!QSystemTrayIcon::isSystemTrayAvailable()) {
    return;
  }

  contextMenu_ = menu;

  icon_ = new QSystemTrayIcon(parent);
  icon_->setContextMenu(contextMenu_);
  icon_->setIcon(baseIcon_);
  icon_->setToolTip("Taiga");
  icon_->show();

  connect(icon_, &QSystemTrayIcon::activated, this,
          [this](QSystemTrayIcon::ActivationReason reason) {
            switch (reason) {
              case QSystemTrayIcon::ActivationReason::Trigger:
              case QSystemTrayIcon::ActivationReason::DoubleClick:
              case QSystemTrayIcon::ActivationReason::MiddleClick:
                emit activated();
                break;
            }
          });

  connect(icon_, &QSystemTrayIcon::messageClicked, this, &TrayIcon::messageClicked);
}

bool TrayIcon::isAvailable() const {
  return icon_ != nullptr;
}

void TrayIcon::setBadge(Badge badge) {
  if (badge_ == badge) return;

  badge_ = badge;
  updateIcon();
}

void TrayIcon::showMessage(const QString& title, const QString& message) {
  if (!icon_) return;

  icon_->showMessage(title, message, QSystemTrayIcon::Information);
}

void TrayIcon::updateIcon() {
  if (!icon_) return;

  if (badge_ == Badge::None) {
    icon_->setIcon(baseIcon_);
    return;
  }

  const auto sizes = baseIcon_.availableSizes();
  const QSize size = sizes.isEmpty() ? QSize(64, 64) : sizes.front();

  QPixmap pixmap = baseIcon_.pixmap(size);
  paintBadge(pixmap);
  icon_->setIcon(QIcon(pixmap));
}

void TrayIcon::paintBadge(QPixmap& pixmap) const {
  const auto color = badge_ == Badge::Success ? Theme::successColor() : Theme::errorColor();

  const QSize pixmapSize = pixmap.size();
  const int size = pixmapSize.width() / 2;
  const QRect rect(pixmapSize.width() - size, pixmapSize.height() - size, size, size);

  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setBrush(color);
  painter.drawEllipse(rect);
}

}  // namespace gui
