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

#include "application.hpp"

#include <QDir>
#include <QFileInfo>
#include <QLocalSocket>
#include <QTimer>
#include <QTranslator>
#include <chrono>
#include <format>

#include "base/log.hpp"
#include "base/string.hpp"
#include "gui/main/main_window.hpp"
#include "gui/utils/theme.hpp"
#include "media/anime_db.hpp"
#include "media/anime_history.hpp"
#include "sync/queue.hpp"
#include "taiga/config.h"
#include "taiga/path.hpp"
#include "taiga/settings.hpp"
#include "taiga/version.hpp"
#include "track/media.hpp"

namespace taiga {

Application::Application(int argc, char* argv[])
    : QApplication(argc, argv), shared_memory_(TAIGA_APP_NAME) {
  setApplicationName("taiga");
  setApplicationDisplayName("Taiga");
  setApplicationVersion(QString::fromStdString(taiga::version().to_string()));
  setOrganizationDomain("taiga.moe");
  setOrganizationName("erengy");
}

Application::~Application() {
  if (window_) {
    window_->hide();
  }
}

int Application::run() {
  parseCommandLine();

  initLogger();

  const auto version = taiga::version().to_string();
  const auto fileInfo = QFileInfo{QCoreApplication::applicationFilePath()};
  const auto lastModified = fileInfo.lastModified().toString(Qt::DateFormat::ISODate);
  qDebug() << u"Version %1 (%2)"_s.arg(version).arg(lastModified);
  if (!parser_.optionNames().isEmpty()) {
    qDebug() << "Options:" << parser_.optionNames().join(", ");
  }

  if (hasPreviousInstance()) {
    activatePreviousInstance();
    qDebug() << "Another instance of Taiga is running.";
    return 0;
  }

  connect(&local_server_, &QLocalServer::newConnection, this, &Application::onNewConnection);
  local_server_.listen(TAIGA_APP_NAME);

  taiga::settings.init();
  anime::db.init();
  anime::history.init();
  sync::queue.init();
  track::media::detection()->init();

  gui::theme.initStyle();
  setWindowIcon(gui::theme.getIcon("taiga", "png"));

  QTranslator translator;
  if (translator.load(QLocale::system(), "taiga", "_", ":/i18n")) {
    installTranslator(&translator);
  }

  window_ = new gui::MainWindow();
  window_->init();

#ifdef Q_OS_WINDOWS
  // Delay showing the window to avoid a white flash.
  window_->setWindowOpacity(0.0);
  window_->show();
  QTimer::singleShot(std::chrono::milliseconds(50), window_, [this]() {
    if (window_) {
      window_->setWindowOpacity(1.0);
    }
  });
#else
  window_->show();
#endif

  return QApplication::exec();
}

bool Application::isDebug() const {
  return options_.debug;
}

bool Application::isVerbose() const {
  return options_.verbose;
}

gui::MainWindow* Application::mainWindow() const {
  return window_.get();
}

bool Application::hasPreviousInstance() {
  return !shared_memory_.create(1);
}

void Application::activatePreviousInstance() {
  QLocalSocket socket;
  socket.connectToServer(TAIGA_APP_NAME);
  socket.waitForConnected(std::chrono::milliseconds(1000).count());
}

void Application::initLogger() const {
  const auto directory = u"%1/logs"_s.arg(get_data_path());
  QDir().mkpath(directory);

  const auto date = QDate::currentDate().toString(Qt::DateFormat::ISODate);
  const auto path = u"%1/%2_%3.log"_s.arg(directory).arg(TAIGA_APP_NAME).arg(date);

  base::initLogging(path, options_.debug ? QtDebugMsg : QtWarningMsg);
}

void Application::onNewConnection() {
  while (auto socket = local_server_.nextPendingConnection()) {
    socket->deleteLater();
  }
  if (window_) {
    window_->displayWindow();
  }
}

void Application::parseCommandLine() {
  parser_.addOptions({
      {"debug", QCoreApplication::translate("main", "Enable debug mode")},
      {"verbose", QCoreApplication::translate("main", "Enable verbose output")},
  });

  // This stops the current process in case of an error (e.g. an unknown option was passed).
  parser_.process(QApplication::arguments());

#ifdef _DEBUG
  options_.debug = true;
#else
  options_.debug = parser_.isSet("debug");
#endif
  options_.verbose = parser_.isSet("verbose");
}

}  // namespace taiga
