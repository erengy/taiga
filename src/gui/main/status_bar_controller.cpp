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

#include "status_bar_controller.hpp"

#include <QStatusBar>

#include "gui/common/spinner_widget.hpp"

namespace gui {

StatusBarController::StatusBarController(QObject* parent, QStatusBar* statusBar,
                                         SpinnerWidget* spinner)
    : QObject(parent), statusBar_(statusBar), spinner_(spinner) {}

void StatusBarController::showMessage(const Message& message) {
  messages_[message.source] = message.text;

  if (message.spin) {
    spinningSources_.insert(message.source);
  } else {
    spinningSources_.remove(message.source);
  }

  refresh();
}

void StatusBarController::clearMessage(Source source) {
  messages_.remove(source);
  spinningSources_.remove(source);
  refresh();
}

void StatusBarController::clearAll() {
  messages_.clear();
  spinningSources_.clear();
  refresh();
}

void StatusBarController::refresh() {
  if (messages_.isEmpty()) {
    statusBar_->clearMessage();
  } else {
    // First entry is the highest-priority source. This relies on
    // `messages_` being sorted by key.
    statusBar_->showMessage(messages_.first());
  }

  if (spinningSources_.isEmpty()) {
    spinner_->stop();
  } else {
    spinner_->start();
  }
}

}  // namespace gui
