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

#include "gui/main/main_window.hpp"
#include "gui/utils/theme.hpp"

namespace taiga {

Application::Application(int argc, char* argv[]) : QApplication(argc, argv) {
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

}  // namespace taiga
