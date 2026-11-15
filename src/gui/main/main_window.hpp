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

#include <QMainWindow>

#include "gui/main/navigation_controller.hpp"

class QLineEdit;

namespace Ui {
class MainWindow;
}

namespace anime::list {
enum class Status;
}

namespace gui {

class HistoryWidget;
class LibraryWidget;
class ListWidget;
class NavigationWidget;
class NowPlayingWidget;
class SearchWidget;
class StatusBarController;
class TrayIcon;

class MainWindow final : public QMainWindow {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(MainWindow)

  friend class NavigationController;

public:
  MainWindow();

  NavigationWidget* navigation() const;
  NowPlayingWidget* nowPlaying() const;
  QLineEdit* searchBox() const;
  StatusBarController* statusBarController() const;
  Ui::MainWindow* ui() const;

  void init();

  bool canHideToTray() const;

public slots:
  void addNewFolder();
  void displayWindow();
  void navigateTo(gui::MainWindowPage page);
  void navigateToListStatus(anime::list::Status status);
  void updateTitle();

private slots:
  void about();
  void donate() const;
  void support() const;
  void synchronize();
  void profile();

protected:
  void closeEvent(QCloseEvent* event) override;

private:
  void initActions();
  void initIcons();
  void initNavigation();
  void initNowPlaying();
  void initPage(MainWindowPage page);
  void initStatusbar();
  void initToolbar();
  void initTrayIcon();

  Ui::MainWindow* ui_ = nullptr;

  HistoryWidget* historyWidget_ = nullptr;
  LibraryWidget* libraryWidget_ = nullptr;
  ListWidget* listWidget_ = nullptr;
  NavigationController* navigationController_ = nullptr;
  NavigationWidget* navigationWidget_ = nullptr;
  NowPlayingWidget* nowPlayingWidget_ = nullptr;
  QLineEdit* searchBox_ = nullptr;
  SearchWidget* searchWidget_ = nullptr;
  StatusBarController* statusBarController_ = nullptr;
  TrayIcon* trayIcon_ = nullptr;
};

MainWindow* mainWindow();

}  // namespace gui
