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

#include "gui/utils/theme.hpp"

namespace gui {

TrayIcon::TrayIcon(QObject* parent, const QIcon& icon, QMenu* menu) : m_baseIcon(icon) {
  if (!QSystemTrayIcon::isSystemTrayAvailable()) {
    return;
  }

  m_contextMenu = menu;

  m_icon = new QSystemTrayIcon(parent);
  m_icon->setContextMenu(m_contextMenu);
  m_icon->setIcon(m_baseIcon);
  m_icon->setToolTip("Taiga");
  m_icon->show();

  connect(m_icon, &QSystemTrayIcon::activated, this,
          [this](QSystemTrayIcon::ActivationReason reason) {
            switch (reason) {
              case QSystemTrayIcon::ActivationReason::Trigger:
              case QSystemTrayIcon::ActivationReason::DoubleClick:
              case QSystemTrayIcon::ActivationReason::MiddleClick:
                emit activated();
                break;
            }
          });

  connect(m_icon, &QSystemTrayIcon::messageClicked, this, &TrayIcon::messageClicked);
}

void TrayIcon::setBadge(Badge badge) {
  if (m_badge == badge) return;

  m_badge = badge;
  updateIcon();
}

void TrayIcon::showMessage(const QString& title, const QString& message) {
  if (!m_icon) return;

  m_icon->showMessage(title, message, QSystemTrayIcon::Information);
}

void TrayIcon::updateIcon() {
  if (!m_icon) return;

  if (m_badge == Badge::None) {
    m_icon->setIcon(m_baseIcon);
    return;
  }

  const auto sizes = m_baseIcon.availableSizes();
  const QSize size = sizes.isEmpty() ? QSize(64, 64) : sizes.front();

  QPixmap pixmap = m_baseIcon.pixmap(size);
  paintBadge(pixmap);
  m_icon->setIcon(QIcon(pixmap));
}

void TrayIcon::paintBadge(QPixmap& pixmap) const {
  const auto color = m_badge == Badge::Success ? Theme::successColor() : Theme::errorColor();

  const QSize pixmapSize = pixmap.size();
  const int size = pixmapSize.width() / 2;
  const QRect rect(pixmapSize.width() - size, pixmapSize.height() - size, size, size);

  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setBrush(color);
  painter.drawEllipse(rect);
}

}  // namespace gui
