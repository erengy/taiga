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

#include <QCommandLineParser>

#include "base/log.hpp"
#include "gui/main/main_window.hpp"
#include "gui/utils/theme.hpp"
#include "taiga/config.h"
#include "taiga/version.hpp"

namespace taiga {

Application::Application(int argc, char* argv[]) : QApplication(argc, argv) {
  setApplicationName("taiga");
  setApplicationDisplayName("Taiga");
  setApplicationVersion(QString::fromStdString(taiga::version().to_string()));

  parseCommandLine();
  initLogger();

  gui::theme.initStyle();

  window_ = new gui::MainWindow();
  window_->show();
}

Application::~Application() {
  if (window_) {
    window_->hide();
  }
}

int Application::run() const {
  return QApplication::exec();
}

bool Application::isDebug() const {
  return options_.debug;
}

bool Application::isVerbose() const {
  return options_.verbose;
}

void Application::initLogger() const {
  using monolog::Level;

  const auto path =
      u"%1/%2.log"_qs.arg(QCoreApplication::applicationDirPath(), TAIGA_APP_NAME).toStdString();

  monolog::log.enable_console_output(false);
  monolog::log.set_path(path);
  monolog::log.set_level(options_.debug ? Level::Debug : Level::Warning);

  LOGI("Version {}", taiga::version().to_string());
}

void Application::parseCommandLine() {
  QCommandLineParser parser;

  parser.addOptions({
      {"debug", QCoreApplication::translate("main", "Enable debug mode")},
      {"verbose", QCoreApplication::translate("main", "Enable verbose output")},
  });

  parser.process(QApplication::arguments());

  // @TODO: Log valid & invalid arguments

#ifdef _DEBUG
  options_.debug = true;
#else
  options_.debug = parser.isSet("debug");
#endif
  options_.verbose = parser.isSet("verbose");
}

}  // namespace taiga
