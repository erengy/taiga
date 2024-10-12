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

#pragma once

#include <QTreeWidget>

namespace anime::list {
enum class Status;
}

namespace gui {

enum class MainWindowPage;
enum class NavigationItemDataRole;

class MainWindow;

class NavigationWidget final : public QTreeWidget {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(NavigationWidget)

public:
  NavigationWidget(MainWindow* mainWindow);
  ~NavigationWidget() = default;

  QTreeWidgetItem* findItemByPage(MainWindowPage page) const;

public slots:
  void refresh();

signals:
  void currentPageChanged(MainWindowPage page);
  void currentListStatusChanged(anime::list::Status status);

protected:
  void mouseMoveEvent(QMouseEvent* event) override;

private:
  QTreeWidgetItem* addItem(const QString& text, const QString& icon, MainWindowPage page);
  QTreeWidgetItem* addChildItem(QTreeWidgetItem* parent, const QString& text);
  void addSeparator();
  void setItemData(QTreeWidgetItem* item, NavigationItemDataRole role, const QVariant& value);

  MainWindow* m_mainWindow = nullptr;
};

}  // namespace gui
