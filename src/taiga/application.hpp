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

#include <QApplication>
#include <QPointer>

namespace gui {
class MainWindow;
}

namespace taiga {

namespace detail {

struct CommandLineOptions {
  bool debug = false;
  bool verbose = false;
};

}  // namespace detail

class Application final : public QApplication {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Application)

public:
  Application(int argc, char* argv[]);
  ~Application() override;

  int run() const;

  bool isDebug() const;
  bool isVerbose() const;

private:
  void initLogger() const;
  void parseCommandLine();

  detail::CommandLineOptions options_;
  QPointer<gui::MainWindow> window_;
};

}  // namespace taiga
