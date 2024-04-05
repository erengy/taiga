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

#include "main_window.hpp"

#include <QDesktopServices>
#include <QFileDialog>
#include <QtWidgets>

#include "taiga/config.h"
#include "ui_main_window.h"

#ifdef Q_OS_WINDOWS
#include "gui/platforms/windows.hpp"
#endif

namespace gui {

MainWindow::MainWindow() : QMainWindow(), ui_(new Ui::MainWindow) {
  ui_->setupUi(this);

#ifdef Q_OS_WINDOWS
  const auto hwnd = reinterpret_cast<HWND>(winId());
  enableMicaBackground(hwnd);
  enableDarkMode(hwnd);
  setStyleSheet("QMainWindow { background: transparent; }");
#endif

  updateTitle();

  initActions();
  initToolbar();
}

void MainWindow::initActions() {
  ui_->actionProfile->setToolTip(tr("%1 (%2)").arg("erengy").arg("AniList"));
  ui_->actionSynchronize->setToolTip(tr("Synchronize with %1").arg("AniList"));

  connect(ui_->actionAddNewFolder, &QAction::triggered, this, &MainWindow::addNewFolder);
  connect(ui_->actionExit, &QAction::triggered, this, &QWidget::close);
  connect(ui_->actionAbout, &QAction::triggered, this, &MainWindow::about);
  connect(ui_->actionDonate, &QAction::triggered, this, &MainWindow::donate);
  connect(ui_->actionSupport, &QAction::triggered, this, &MainWindow::support);
  connect(ui_->actionProfile, &QAction::triggered, this, &MainWindow::profile);
  connect(ui_->actionDisplayWindow, &QAction::triggered, this, &MainWindow::displayWindow);

  connect(ui_->actionSynchronize, &QAction::triggered, this, [this]() {
    setEnabled(false);
    statusBar()->showMessage("Synchronizing with AniList...");
    QEventLoop loop;
    QTimer::singleShot(3000, &loop, [this, &loop]() {
      setEnabled(true);
      statusBar()->clearMessage();
      loop.quit();
    });
    loop.exec();
  });
}

void MainWindow::initToolbar() {
  ui_->toolbar->setIconSize(QSize{24, 24});

  {
    const auto button = static_cast<QToolButton*>(ui_->toolbar->widgetForAction(ui_->actionMenu));
    button->setPopupMode(QToolButton::InstantPopup);
    button->setMenu([this]() {
      auto menu = new QMenu(this);
      menu->addAction(ui_->actionToggleDetection);
      menu->addAction(ui_->actionToggleSharing);
      menu->addAction(ui_->actionToggleSynchronization);
      menu->addSeparator();
      menu->addMenu(ui_->menuLibrary);
      menu->addMenu(ui_->menuHelp);
      menu->addSeparator();
      menu->addAction(ui_->actionExit);
      return menu;
    }());
  }
}

void MainWindow::addNewFolder() {
  constexpr auto options =
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks | QFileDialog::ReadOnly;

  const auto directory = QFileDialog::getExistingDirectory(this, tr("Add New Folder"), "", options);

  if (!directory.isEmpty()) {
    QMessageBox::information(this, "New Folder", directory);
  }
}

void MainWindow::setPage(int index) {
  ui_->stackedWidget->setCurrentIndex(index);
}

void MainWindow::updateTitle() {
  auto title = u"Taiga"_qs;

#ifdef _DEBUG
  title += u" [debug]"_qs;
#endif

  setWindowTitle(title);
}

void MainWindow::displayWindow() {
  setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
  activateWindow();
}

void MainWindow::about() {
  const QVersionNumber versionNumber(TAIGA_VERSION_MAJOR, TAIGA_VERSION_MINOR, TAIGA_VERSION_PATCH);
  QString version = versionNumber.toString();
  if (QString pre(TAIGA_VERSION_PRE); !pre.isEmpty()) {
    version += "-" + pre;
  }

  QString text =
      tr("<b>Taiga</b> v%1<br><br>This version is a work in progress. For more "
         "information, visit the <a href=\"https://github.com/erengy/taiga\">GitHub repository</a> "
         "or the <a href=\"https://discord.gg/yeGNktZ\">Discord server</a>.")
          .arg(version);

  QMessageBox::about(this, tr("About Taiga"), text);
}

void MainWindow::donate() const {
  QDesktopServices::openUrl(QUrl("https://taiga.moe/#donate"));
}

void MainWindow::support() const {
  QDesktopServices::openUrl(QUrl("https://taiga.moe/#support"));
}

void MainWindow::profile() const {
  QDesktopServices::openUrl(QUrl("https://anilist.co/user/erengy/"));
}

}  // namespace gui
