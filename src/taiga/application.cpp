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

#include "application.hpp"

#include <QDir>
#include <QFileInfo>
#include <QLocalSocket>
#include <QTimer>
#include <QTranslator>
#include <format>

#include "base/log.hpp"
#include "gui/main/main_window.hpp"
#include "gui/utils/theme.hpp"
#include "media/anime_db.hpp"
#include "media/anime_history.hpp"
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

  LOGD("Version {} ({})", taiga::version().to_string(),
       QFileInfo{QCoreApplication::applicationFilePath()}
           .lastModified()
           .toString(Qt::DateFormat::ISODate)
           .toStdString());
  if (!parser_.optionNames().isEmpty()) {
    LOGD("Options: {}", parser_.optionNames().join(", ").toStdString());
  }

  if (hasPreviousInstance()) {
    activatePreviousInstance();
    LOGD("Another instance of Taiga is running.");
    return 0;
  }

  connect(&local_server_, &QLocalServer::newConnection, this, &Application::onNewConnection);
  local_server_.listen(TAIGA_APP_NAME);

  taiga::settings.init();
  anime::db.init();
  anime::history.init();
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
  QTimer::singleShot(50, window_, [this]() {
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
  socket.waitForConnected(1000);
}

void Application::initLogger() const {
  using monolog::Level;

  const auto directory = std::format("{}/logs", get_data_path());
  QDir().mkpath(QString::fromStdString(directory));

  const auto date = QDate::currentDate().toString(Qt::DateFormat::ISODate).toStdString();
  const auto path = std::format("{}/{}_{}.log", directory, TAIGA_APP_NAME, date);

  monolog::log.enable_console_output(false);
  monolog::log.set_path(path);
  monolog::log.set_level(options_.debug ? Level::Debug : Level::Warning);
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
