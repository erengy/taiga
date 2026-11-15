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
#include <algorithm>
#include <optional>

#include "base/string.hpp"
#include "gui/common/spinner_widget.hpp"
#include "gui/history/history_widget.hpp"
#include "gui/library/library_widget.hpp"
#include "gui/list/list_widget.hpp"
#include "gui/main/about_dialog.hpp"
#include "gui/main/navigation_controller.hpp"
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
#include "track/media.hpp"
#include "track/update_session.hpp"
#include "ui_main_window.h"

#ifdef Q_OS_WINDOWS
#include "gui/platforms/windows.hpp"
#endif

namespace gui {

MainWindow::MainWindow() : ui_(new Ui::MainWindow) {
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
  return navigationWidget_;
}

NowPlayingWidget* MainWindow::nowPlaying() const {
  return nowPlayingWidget_;
}

QLineEdit* MainWindow::searchBox() const {
  return searchBox_;
}

StatusBarController* MainWindow::statusBarController() const {
  return statusBarController_;
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
  ui_->actionProfile->setDisabled(true);  // placeholder
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

  ui_->actionToggleDetection->setChecked(track::media::detection()->isEnabled());
  connect(ui_->actionToggleDetection, &QAction::toggled, this,
          [](const bool checked) { taiga::settings()->setDetectionEnabled(checked); });

  ui_->actionToggleSharing->setChecked(taiga::settings()->sharingEnabled());
  connect(ui_->actionToggleSharing, &QAction::toggled, this,
          [](const bool checked) { taiga::settings()->setSharingEnabled(checked); });

  ui_->actionToggleSynchronization->setChecked(taiga::settings()->syncEnabled());
  connect(ui_->actionToggleSynchronization, &QAction::toggled, this,
          [](const bool checked) { taiga::settings()->setSyncEnabled(checked); });

  // Settings can also be changed from the settings dialog.
  connect(taiga::settings(), &taiga::Settings::changed, this, [this](const QSet<QString>& keys) {
    if (keys.contains(taiga::Settings::Key::kDetectionEnabled)) {
      ui_->actionToggleDetection->setChecked(taiga::settings()->detectionEnabled());
    }
    if (keys.contains(taiga::Settings::Key::kSharingEnabled)) {
      ui_->actionToggleSharing->setChecked(taiga::settings()->sharingEnabled());
    }
    if (keys.contains(taiga::Settings::Key::kSyncEnabled)) {
      ui_->actionToggleSynchronization->setChecked(taiga::settings()->syncEnabled());
    }
  });
}

void MainWindow::initIcons() {
  ui_->menuLibraryFolders->setIcon(theme()->getIcon("folder"));
  ui_->menuExport->setIcon(theme()->getIcon("export_notes"));

  ui_->actionAddNewFolder->setIcon(theme()->getIcon("create_new_folder"));
  ui_->actionAbout->setIcon(theme()->getIcon("info"));
  ui_->actionBack->setIcon(theme()->getIcon("arrow_back"));
  ui_->actionCheckForUpdates->setIcon(theme()->getIcon("cloud_download"));
  ui_->actionDonate->setIcon(theme()->getIcon("favorite"));
  ui_->actionExit->setIcon(theme()->getIcon("logout"));
  ui_->actionForward->setIcon(theme()->getIcon("arrow_forward"));
  ui_->actionMenu->setIcon(theme()->getIcon("menu"));
  ui_->actionPlayNextEpisode->setIcon(theme()->getIcon("skip_next"));
  ui_->actionPlayRandomAnime->setIcon(theme()->getIcon("shuffle"));
  ui_->actionProfile->setIcon(theme()->getIcon("account_circle"));
  ui_->actionScanAvailableEpisodes->setIcon(theme()->getIcon("pageview"));
  ui_->actionSettings->setIcon(theme()->getIcon("settings"));
  ui_->actionSupport->setIcon(theme()->getIcon("help"));
  ui_->actionSynchronize->setIcon(theme()->getIcon("sync"));
}

void MainWindow::initNavigation() {
  navigationWidget_ = new NavigationWidget(this);
  // Connects to navigationWidget_'s signals on construction, so it must come after.
  navigationController_ = new NavigationController(this);

  const bool hasWatching = std::ranges::any_of(anime::db()->entries(), [](const auto& entry) {
    return entry.status == anime::list::Status::Watching;
  });
  if (hasWatching) {
    navigateToListStatus(anime::list::Status::Watching);
  } else {
    navigateTo(MainWindowPage::List);
  }

  ui_->splitter->insertWidget(0, navigationWidget_);
}

void MainWindow::initNowPlaying() {
  nowPlayingWidget_ = new NowPlayingWidget(ui_->centralWidget);

  ui_->centralWidget->layout()->addWidget(nowPlayingWidget_);
  nowPlayingWidget_->hide();
}

void MainWindow::initPage(MainWindowPage page) {
  static QSet<MainWindowPage> initializedPages;

  if (initializedPages.contains(page)) return;

  static const auto initPage = [](QWidget* page, QWidget* widget) {
    const auto layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(widget);
  };

  switch (page) {
    case MainWindowPage::Home:
      break;

    case MainWindowPage::Search:
      searchWidget_ = new SearchWidget(ui_->searchPage);
      initPage(ui_->searchPage, searchWidget_);
      break;

    case MainWindowPage::List:
      listWidget_ = new ListWidget(ui_->listPage);
      initPage(ui_->listPage, listWidget_);
      break;

    case MainWindowPage::History:
      historyWidget_ = new HistoryWidget(ui_->historyPage);
      initPage(ui_->historyPage, historyWidget_);
      break;

    case MainWindowPage::Library:
      libraryWidget_ = new LibraryWidget(ui_->libraryPage);
      initPage(ui_->libraryPage, libraryWidget_);
      break;

    // NOLINTNEXTLINE(bugprone-branch-clone)
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

  statusBarController_ = new StatusBarController(this, statusbar, spinner);

  const QList<sync::Service*> services{
      sync::anilist::Service::instance(),
      sync::kitsu::Service::instance(),
      sync::myanimelist::Service::instance(),
  };
  for (auto* service : services) {
    connect(service, &sync::Service::authenticationCompleted, this,
            [this](const bool authenticated) {
              if (!authenticated) return;

              const auto senderService = qobject_cast<sync::Service*>(sender());
              const auto slug = sync::serviceSlug(senderService->id()).toStdString();
              const auto username = taiga::accounts()->serviceUsername(slug);

              statusBarController_->showMessage({
                  .source = StatusBarController::Source::Sync,
                  .text = tr("Logged in as %1.").arg(username),
                  .spin = false,
              });
            });
    connect(service, &sync::Service::listEntriesFetched, this, [this]() {
      statusBarController_->clearMessage(StatusBarController::Source::Sync);
      setEnabled(true);
    });
    connect(service, &sync::Service::errorOccurred, this, [this](const QString& message) {
      const auto senderService = qobject_cast<sync::Service*>(sender());
      statusBarController_->showMessage({
          .source = StatusBarController::Source::Sync,
          .text = sync::tagMessage(senderService->id(), message),
          .spin = false,
      });
      setEnabled(true);
    });
    connect(service, &sync::Service::transferProgress, this,
            [this](const qint64 current, const qint64 total) {
              statusBarController_->showMessage({
                  .source = StatusBarController::Source::Sync,
                  .text = tr("Synchronizing with %1... (%2)")
                              .arg(sync::serviceName(sync::currentServiceId()))
                              .arg(gui::formatTransferProgress(current, total)),
              });
            });
  }

  connect(sync::queue(), &sync::Queue::changed, this, [this]() {
    if (sync::queue()->count() == 0) {
      statusBarController_->clearMessage(StatusBarController::Source::Sync);
      setEnabled(true);
    }
  });

  connect(sync::queue(), &sync::Queue::processing, this, [this](const int animeId) {
    const auto item = anime::db()->item(animeId);
    const auto entry = anime::db()->entry(animeId);
    if (!item || !entry) return;

    const auto title = anime::preferredTitle(*item);

    QString text;
    if (entry->pendingDelete) {
      text = tr("Deleting list entry... (%1)").arg(title);
    } else if (entry->id == anime::list::kUnknownId) {
      text = tr("Adding to list... (%1)").arg(title);
    } else {
      text = tr("Updating list entry... (%1)").arg(title);
    }

    statusBarController_->showMessage({
        .source = StatusBarController::Source::Sync,
        .text = text,
    });
  });

  connect(sync::queue(), &sync::Queue::queuedWhileUnauthenticated, this, [this](const int animeId) {
    const auto item = anime::db()->item(animeId);
    if (!item) return;

    statusBarController_->showMessage({
        .source = StatusBarController::Source::Sync,
        .text = tr("%1 is queued for update.").arg(anime::preferredTitle(*item)),
        .spin = false,
    });
  });

  connect(anime::db(), &anime::Database::itemDeleted, this,
          [this](const int, const QString& title) {
            if (title.isEmpty()) return;

            statusBarController_->showMessage({
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
    searchBox_ = new QLineEdit();
    searchBox_->setClearButtonEnabled(true);
    searchBox_->setFixedWidth(320);
    searchBox_->setPlaceholderText(tr("Search"));

    const auto before = ui_->actionSettings;
    const auto insertSpacer = [this](QAction* before) {
      ui_->toolbar->insertWidget(before, [this]() {
        auto spacer = new QWidget(this);
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        return spacer;
      }());
    };

    insertSpacer(before);
    ui_->toolbar->insertWidget(before, searchBox_);
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

  trayIcon_ = new TrayIcon(this, windowIcon(), menu);

  connect(trayIcon_, &TrayIcon::activated, this, &MainWindow::displayWindow);
  connect(trayIcon_, &TrayIcon::messageClicked, this, &MainWindow::displayWindow);

  connect(track::updateSession(), &track::UpdateSession::confirmationRequested, this,
          [this](const track::UpdateState& state) {
            if (isActiveWindow()) return;

            const auto episode = track::media::detection()->getCurrentEpisode();
            const auto item = episode ? anime::db()->item(episode->animeId()) : nullptr;
            if (!item) return;

            trayIcon_->showMessage(tr("Confirm list update"),
                                   tr("%1: episode %2 is ahead of your progress (%3).")
                                       .arg(QString::fromStdString(anime::preferredTitle(*item)))
                                       .arg(state.episode)
                                       .arg(state.previousEpisode));
          });

  connect(track::media::detection(), &track::media::Detection::currentEpisodeChanged, this,
          [this](std::optional<track::Episode> episode) {
            if (!episode) {
              trayIcon_->setBadge(TrayIcon::Badge::None);
            } else {
              const bool recognized = anime::db()->item(episode->animeId()) != nullptr;
              trayIcon_->setBadge(recognized ? TrayIcon::Badge::Success : TrayIcon::Badge::Error);
            }
          });
}

void MainWindow::closeEvent(QCloseEvent* event) {
  taiga::session.setMainWindowGeometry(saveGeometry());
  if (listWidget_) listWidget_->saveState();
  if (searchWidget_) searchWidget_->saveState();

  // Closed by the user, rather than by quitting the application.
  if (event->spontaneous()) {
    if (taiga::settings()->appCloseToTray() && canHideToTray()) {
      hide();
      event->ignore();
      if (!taiga::session.mainWindowTrayNoticeShown()) {
        taiga::session.setMainWindowTrayNoticeShown(true);
        trayIcon_->showMessage(tr("Taiga is still running"),
                               tr("Taiga keeps detecting what you watch in the background. "
                                  "To quit, use Exit from the tray menu."));
      }
      return;
    }
    QMetaObject::invokeMethod(qApp, &QApplication::quit, Qt::QueuedConnection);
  }

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
  navigationController_->navigateTo(page);
}

void MainWindow::navigateToListStatus(anime::list::Status status) {
  navigationController_->navigateToListStatus(status);
}

void MainWindow::updateTitle() {
  auto title = u"Taiga"_s;

  if (taiga::app()->isDebug()) {
    title += u" [debug]"_s;
  }

  setWindowTitle(title);
}

void MainWindow::displayWindow() {
  show();
  setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
  raise();
  activateWindow();
}

bool MainWindow::canHideToTray() const {
  return trayIcon_ && trayIcon_->isAvailable();
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

  statusBarController_->showMessage({
      .source = StatusBarController::Source::Sync,
      .text = text,
  });

  if (!sync::synchronize()) {
    statusBarController_->clearMessage(StatusBarController::Source::Sync);
    setEnabled(true);
  }
}

void MainWindow::profile() {
  navigateTo(MainWindowPage::Profile);
}

}  // namespace gui
