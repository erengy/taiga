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

#include "main_window.hpp"

#include <QDesktopServices>
#include <QFileDialog>
#include <QtWidgets>

#include "base/string.hpp"
#include "gui/common/spinner_widget.hpp"
#include "gui/history/history_widget.hpp"
#include "gui/library/library_widget.hpp"
#include "gui/list/list_widget.hpp"
#include "gui/main/about_dialog.hpp"
#include "gui/main/navigation_widget.hpp"
#include "gui/main/now_playing_widget.hpp"
#include "gui/main/status_bar.hpp"
#include "gui/main/status_bar_controller.hpp"
#include "gui/search/search_widget.hpp"
#include "gui/settings/settings_dialog.hpp"
#include "gui/utils/format.hpp"
#include "gui/utils/theme.hpp"
#include "gui/utils/tray_icon.hpp"
#include "gui/utils/widgets.hpp"
#include "media/anime_db.hpp"
#include "media/anime_list.hpp"
#include "media/anime_utils.hpp"
#include "sync/anilist/anilist.hpp"
#include "sync/kitsu/kitsu.hpp"
#include "sync/myanimelist/myanimelist.hpp"
#include "sync/queue.hpp"
#include "sync/service.hpp"
#include "taiga/accounts.hpp"
#include "taiga/application.hpp"
#include "taiga/session.hpp"
#include "taiga/settings.hpp"
#include "ui_main_window.h"

#ifdef Q_OS_WINDOWS
#include "gui/platforms/windows.hpp"
#endif

namespace gui {

MainWindow::MainWindow() : QMainWindow(), ui_(new Ui::MainWindow) {
  ui_->setupUi(this);

  ui_->menubar->hide();

#ifdef Q_OS_WINDOWS
  enableMicaBackground(this);
#endif

  if (const auto geometry = taiga::session.mainWindowGeometry(); !geometry.isEmpty()) {
    restoreGeometry(geometry);
    centerWidgetToScreen(this);
  }

  // Do not call `init()` here, as it relies on the main window pointer being
  // available through the application instance.
}

MainWindow* mainWindow() {
  return taiga::app()->mainWindow();
}

NavigationWidget* MainWindow::navigation() const {
  return m_navigationWidget;
}

NowPlayingWidget* MainWindow::nowPlaying() const {
  return m_nowPlayingWidget;
}

QLineEdit* MainWindow::searchBox() const {
  return m_searchBox;
}

StatusBarController* MainWindow::statusBarController() const {
  return m_statusBarController;
}

Ui::MainWindow* MainWindow::ui() const {
  return ui_;
}

void MainWindow::init() {
  initActions();
  initIcons();
  initTrayIcon();
  initToolbar();
  initStatusbar();
  initNavigation();
  initNowPlaying();
  updateTitle();
}

void MainWindow::initActions() {
  ui_->actionProfile->setToolTip(tr("Profile"));
  ui_->actionSynchronize->setToolTip(
      tr("Synchronize with %1").arg(sync::serviceName(sync::currentServiceId())));

  connect(ui_->actionAddNewFolder, &QAction::triggered, this, &MainWindow::addNewFolder);
  connect(ui_->actionExit, &QAction::triggered, this, &QApplication::quit, Qt::QueuedConnection);
  connect(ui_->actionSettings, &QAction::triggered, this, [this]() { SettingsDialog::show(this); });
  connect(ui_->actionAbout, &QAction::triggered, this, &MainWindow::about);
  connect(ui_->actionDonate, &QAction::triggered, this, &MainWindow::donate);
  connect(ui_->actionSupport, &QAction::triggered, this, &MainWindow::support);
  connect(ui_->actionProfile, &QAction::triggered, this, &MainWindow::profile);
  connect(ui_->actionDisplayWindow, &QAction::triggered, this, &MainWindow::displayWindow);
  connect(ui_->actionSynchronize, &QAction::triggered, this, &MainWindow::synchronize);

  ui_->actionToggleSynchronization->setChecked(taiga::settings.syncEnabled());
  connect(ui_->actionToggleSynchronization, &QAction::toggled, this,
          [](const bool checked) { taiga::settings.setSyncEnabled(checked); });
}

void MainWindow::initIcons() {
  ui_->menuLibraryFolders->setIcon(theme.getIcon("folder"));
  ui_->menuExport->setIcon(theme.getIcon("export_notes"));

  ui_->actionAddNewFolder->setIcon(theme.getIcon("create_new_folder"));
  ui_->actionAbout->setIcon(theme.getIcon("info"));
  ui_->actionBack->setIcon(theme.getIcon("arrow_back"));
  ui_->actionCheckForUpdates->setIcon(theme.getIcon("cloud_download"));
  ui_->actionDonate->setIcon(theme.getIcon("favorite"));
  ui_->actionExit->setIcon(theme.getIcon("logout"));
  ui_->actionForward->setIcon(theme.getIcon("arrow_forward"));
  ui_->actionLibraryFolders->setIcon(theme.getIcon("folder"));
  ui_->actionMenu->setIcon(theme.getIcon("menu"));
  ui_->actionPlayNextEpisode->setIcon(theme.getIcon("skip_next"));
  ui_->actionPlayRandomAnime->setIcon(theme.getIcon("shuffle"));
  ui_->actionProfile->setIcon(theme.getIcon("account_circle"));
  ui_->actionScanAvailableEpisodes->setIcon(theme.getIcon("pageview"));
  ui_->actionSettings->setIcon(theme.getIcon("settings"));
  ui_->actionSupport->setIcon(theme.getIcon("help"));
  ui_->actionSynchronize->setIcon(theme.getIcon("sync"));
}

void MainWindow::initNavigation() {
  m_navigationWidget = new NavigationWidget(this);

  connect(m_navigationWidget, &NavigationWidget::currentPageChanged, this, &MainWindow::setPage);

  navigateTo(MainWindowPage::List);

  ui_->splitter->insertWidget(0, m_navigationWidget);
}

void MainWindow::initNowPlaying() {
  m_nowPlayingWidget = new NowPlayingWidget(ui_->centralWidget);

  ui_->centralWidget->layout()->addWidget(m_nowPlayingWidget);
  m_nowPlayingWidget->hide();
}

void MainWindow::initPage(MainWindowPage page) {
  static QSet<MainWindowPage> initializedPages;

  if (initializedPages.contains(page)) return;

  static const auto init_page = [](QWidget* page, QWidget* widget) {
    const auto layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(widget);
    page->setLayout(layout);
  };

  switch (page) {
    case MainWindowPage::Home:
      break;

    case MainWindowPage::Search:
      m_searchWidget = new SearchWidget(ui_->searchPage);
      init_page(ui_->searchPage, m_searchWidget);
      break;

    case MainWindowPage::List:
      m_listWidget = new ListWidget(ui_->listPage);
      init_page(ui_->listPage, m_listWidget);
      break;

    case MainWindowPage::History:
      m_historyWidget = new HistoryWidget(ui_->historyPage);
      init_page(ui_->historyPage, m_historyWidget);
      break;

    case MainWindowPage::Library:
      m_libraryWidget = new LibraryWidget(ui_->libraryPage);
      init_page(ui_->libraryPage, m_libraryWidget);
      break;

    case MainWindowPage::Torrents:
      break;

    case MainWindowPage::Profile:
      break;
  }

  initializedPages.insert(page);
}

void MainWindow::initStatusbar() {
  const auto statusbar = new StatusBar(this);
  statusbar->setObjectName(ui_->statusbar->objectName());
  setStatusBar(statusbar);
  ui_->statusbar = statusbar;

  const auto spinner = new SpinnerWidget(this);
  const auto spinnerContainer = new QWidget(this);
  const auto spinnerLayout = new QVBoxLayout(spinnerContainer);
  spinnerLayout->setContentsMargins(0, 2, 0, 0);
  spinnerLayout->addWidget(spinner);
  ui_->statusbar->addPermanentWidget(spinnerContainer);

  m_statusBarController = new StatusBarController(this, statusbar, spinner);

  const QList<sync::Service*> services{
      sync::anilist::Service::instance(),
      sync::kitsu::Service::instance(),
      sync::myanimelist::Service::instance(),
  };
  for (auto* service : services) {
    connect(service, &sync::Service::authenticationCompleted, this,
            [this](const bool authenticated) {
              if (!authenticated) return;

              const auto sender_service = qobject_cast<sync::Service*>(sender());
              const auto slug = sync::serviceSlug(sender_service->id()).toStdString();
              const auto username = taiga::accounts.serviceUsername(slug);

              m_statusBarController->showMessage({
                  .source = StatusBarController::Source::Sync,
                  .text = tr("Logged in as %1.").arg(username),
                  .spin = false,
              });
            });
    connect(service, &sync::Service::listEntriesFetched, this, [this]() {
      m_statusBarController->clearMessage(StatusBarController::Source::Sync);
      setEnabled(true);
    });
    connect(service, &sync::Service::errorOccurred, this, [this](const QString& message) {
      const auto sender_service = qobject_cast<sync::Service*>(sender());
      m_statusBarController->showMessage({
          .source = StatusBarController::Source::Sync,
          .text = sync::tagMessage(sender_service->id(), message),
          .spin = false,
      });
      setEnabled(true);
    });
    connect(service, &sync::Service::transferProgress, this,
            [this](const qint64 current, const qint64 total) {
              m_statusBarController->showMessage({
                  .source = StatusBarController::Source::Sync,
                  .text = tr("Synchronizing with %1... (%2)")
                              .arg(sync::serviceName(sync::currentServiceId()))
                              .arg(gui::formatTransferProgress(current, total)),
              });
            });
  }

  connect(&sync::queue, &sync::Queue::changed, this, [this]() {
    if (sync::queue.count() == 0) {
      m_statusBarController->clearMessage(StatusBarController::Source::Sync);
      setEnabled(true);
    }
  });

  connect(&sync::queue, &sync::Queue::processing, this, [this](const int animeId) {
    const auto item = anime::db.item(animeId);
    const auto entry = anime::db.entry(animeId);
    if (!item || !entry) return;

    const auto title = anime::preferredTitle(*item);

    QString text;
    if (entry->pending_delete) {
      text = tr("Deleting list entry... (%1)").arg(title);
    } else if (entry->id == anime::list::kUnknownId) {
      text = tr("Adding to list... (%1)").arg(title);
    } else {
      text = tr("Updating list entry... (%1)").arg(title);
    }

    m_statusBarController->showMessage({
        .source = StatusBarController::Source::Sync,
        .text = text,
    });
  });

  connect(&sync::queue, &sync::Queue::queuedWhileUnauthenticated, this, [this](const int animeId) {
    const auto item = anime::db.item(animeId);
    if (!item) return;

    m_statusBarController->showMessage({
        .source = StatusBarController::Source::Sync,
        .text = tr("%1 is queued for update.").arg(anime::preferredTitle(*item)),
        .spin = false,
    });
  });

  connect(&anime::db, &anime::Database::itemDeleted, this, [this](const int, const QString& title) {
    if (title.isEmpty()) return;

    m_statusBarController->showMessage({
        .source = StatusBarController::Source::Sync,
        .text = tr("Anime removed from database: %1").arg(title),
        .spin = false,
    });
  });
}

void MainWindow::initToolbar() {
  ui_->toolbar->setIconSize(QSize{24, 24});

  // Menu
  {
    const auto button = static_cast<QToolButton*>(ui_->toolbar->widgetForAction(ui_->actionMenu));
    button->setPopupMode(QToolButton::InstantPopup);
    button->setMenu([this]() {
      auto menu = new QMenu(this);
      menu->addAction(ui_->actionToggleDetection);
      menu->addAction(ui_->actionToggleSharing);
      menu->addAction(ui_->actionToggleSynchronization);
      menu->addSeparator();
      menu->addMenu(ui_->menuHelp);
      menu->addSeparator();
      menu->addAction(ui_->actionExit);
      return menu;
    }());
  }

  // Search box
  {
    m_searchBox = new QLineEdit();
    m_searchBox->setClearButtonEnabled(true);
    m_searchBox->setFixedWidth(320);
    m_searchBox->setPlaceholderText(tr("Search"));

    const auto before = ui_->actionSettings;
    const auto insertSpacer = [this](QAction* before) {
      ui_->toolbar->insertWidget(before, [this]() {
        auto spacer = new QWidget(this);
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        return spacer;
      }());
    };

    insertSpacer(before);
    ui_->toolbar->insertWidget(before, m_searchBox);
    insertSpacer(before);
  }
}

void MainWindow::initTrayIcon() {
  auto menu = new QMenu(this);
  menu->addAction(ui_->actionDisplayWindow);
  menu->setDefaultAction(ui_->actionDisplayWindow);
  menu->addSeparator();
  menu->addAction(ui_->actionSettings);
  menu->addSeparator();
  menu->addAction(ui_->actionExit);

  m_trayIcon = new TrayIcon(this, windowIcon(), menu);

  connect(m_trayIcon, &TrayIcon::activated, this, &MainWindow::displayWindow);
  connect(m_trayIcon, &TrayIcon::messageClicked, this,
          []() { QMessageBox::information(nullptr, "Taiga", tr("Clicked message")); });
}

void MainWindow::closeEvent(QCloseEvent* event) {
  taiga::session.setMainWindowGeometry(saveGeometry());
  if (m_listWidget) m_listWidget->saveState();
  if (m_searchWidget) m_searchWidget->saveState();
  event->accept();
}

void MainWindow::addNewFolder() {
  constexpr auto options =
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks | QFileDialog::ReadOnly;

  const auto directory = QFileDialog::getExistingDirectory(this, tr("Add New Folder"), "", options);

  if (!directory.isEmpty()) {
    QMessageBox::information(this, "New Folder", directory);
  }
}

void MainWindow::navigateTo(MainWindowPage page) {
  if (const auto item = m_navigationWidget->findItemByPage(page)) {
    m_navigationWidget->setCurrentItem(item);
  }
}

void MainWindow::setPage(MainWindowPage page) {
  initPage(page);
  m_statusBarController->clearAll();
  ui_->stackedWidget->setCurrentIndex(static_cast<int>(page));
}

void MainWindow::updateTitle() {
  auto title = u"Taiga"_s;

  if (taiga::app()->isDebug()) {
    title += u" [debug]"_s;
  }

  setWindowTitle(title);
}

void MainWindow::displayWindow() {
  setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
  activateWindow();
}

void MainWindow::about() {
  displayAboutDialog(this);
}

void MainWindow::donate() const {
  QDesktopServices::openUrl(QUrl("https://taiga.moe/#donate"));
}

void MainWindow::support() const {
  QDesktopServices::openUrl(QUrl("https://taiga.moe/#support"));
}

void MainWindow::synchronize() {
  setEnabled(false);

  const auto serviceName = sync::serviceName(sync::currentServiceId());
  const auto text = sync::willAuthenticate() ? tr("Authenticating with %1...").arg(serviceName)
                                             : tr("Synchronizing with %1...").arg(serviceName);

  m_statusBarController->showMessage({
      .source = StatusBarController::Source::Sync,
      .text = text,
  });

  if (!sync::synchronize()) {
    m_statusBarController->clearMessage(StatusBarController::Source::Sync);
    setEnabled(true);
  }
}

void MainWindow::profile() {
  setPage(MainWindowPage::Profile);
  m_navigationWidget->setCurrentIndex({});
}

}  // namespace gui
