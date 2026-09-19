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

#pragma once

#include <QIcon>
#include <QObject>

class QMenu;
class QSystemTrayIcon;

namespace gui {

class TrayIcon final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(TrayIcon)

public:
  enum class Badge {
    None,
    Success,
    Error,
  };

  TrayIcon(QObject* parent, const QIcon& icon, QMenu* menu);

  void setBadge(Badge badge);
  void showMessage(const QString& title, const QString& message);

signals:
  void activated();
  void messageClicked();

private:
  void updateIcon();
  void paintBadge(QPixmap& pixmap) const;

  Badge m_badge = Badge::None;
  QIcon m_baseIcon;
  QMenu* m_contextMenu = nullptr;
  QSystemTrayIcon* m_icon = nullptr;
};

}  // namespace gui
