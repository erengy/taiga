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

#include "tray_icon.hpp"

#include <QIcon>
#include <QMenu>
#include <QSystemTrayIcon>

namespace gui {

TrayIcon::TrayIcon(QObject* parent, const QIcon& icon, QMenu* menu) {
  if (!QSystemTrayIcon::isSystemTrayAvailable()) {
    return;
  }

  m_contextMenu = menu;

  m_icon = new QSystemTrayIcon(parent);
  m_icon->setContextMenu(m_contextMenu);
  m_icon->setIcon(icon);
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

}  // namespace gui
