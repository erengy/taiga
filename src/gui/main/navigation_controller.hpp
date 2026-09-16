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

#include <QList>
#include <QObject>

class QEvent;

namespace anime::list {
enum class Status;
}

namespace gui {

class MainWindow;

enum class MainWindowPage {
  Home,
  Search,
  List,
  History,
  Library,
  Torrents,
  Profile,
};

class NavigationController final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(NavigationController)

public:
  explicit NavigationController(MainWindow* mainWindow);
  ~NavigationController() override = default;

public slots:
  void goBack();
  void goForward();
  void navigateTo(MainWindowPage page, bool addToHistory = true);
  void navigateToListStatus(anime::list::Status status);

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

private:
  void applyPage(MainWindowPage page, bool addToHistory);
  void recordPageHistory(MainWindowPage page);
  void updateHistoryActions();

  MainWindow* m_mainWindow;

  QList<MainWindowPage> m_pageHistory;
  int m_pageHistoryIndex = -1;
};

}  // namespace gui
