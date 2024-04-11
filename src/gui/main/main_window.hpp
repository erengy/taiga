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

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

namespace gui {

class NavigationWidget;
class TrayIcon;

enum class MainWindowPage {
  Home,
  Search,
  List,
  History,
  Library,
  Torrents,
};

class MainWindow final : public QMainWindow {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(MainWindow)

public:
  MainWindow();
  ~MainWindow() = default;

public slots:
  void addNewFolder();
  void displayWindow();
  void setPage(int index);
  void updateTitle();

private slots:
  void about();
  void donate() const;
  void support() const;
  void profile() const;

private:
  void initActions();
  void initIcons();
  void initNavigation();
  void initToolbar();

  Ui::MainWindow* ui_ = nullptr;

  NavigationWidget* m_navigationWidget = nullptr;
  TrayIcon* m_trayIcon = nullptr;
};

}  // namespace gui
