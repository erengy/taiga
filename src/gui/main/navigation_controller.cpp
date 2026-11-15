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

#include "navigation_controller.hpp"

#include <QAction>
#include <QApplication>
#include <QMouseEvent>
#include <QSignalBlocker>

#include "gui/main/main_window.hpp"
#include "gui/main/navigation_widget.hpp"
#include "gui/main/status_bar_controller.hpp"
#include "ui_main_window.h"

namespace gui {

NavigationController::NavigationController(MainWindow* mainWindow)
    : QObject(mainWindow), mainWindow_(mainWindow) {
  connect(mainWindow->navigation(), &NavigationWidget::currentPageChanged, this,
          [this](MainWindowPage page) { applyPage(page, /*addToHistory=*/true); });

  connect(mainWindow->ui()->actionBack, &QAction::triggered, this, &NavigationController::goBack);
  connect(mainWindow->ui()->actionForward, &QAction::triggered, this,
          &NavigationController::goForward);

  qApp->installEventFilter(this);
}

void NavigationController::navigateTo(MainWindowPage page, bool addToHistory) {
  const auto navigation = mainWindow_->navigation();
  const auto item = navigation->findItemByPage(page);

  if (item && addToHistory) {
    navigation->setCurrentItem(item);
    return;  // `currentPageChanged` signal handles the rest
  }

  const QSignalBlocker blocker(navigation);
  if (item) {
    navigation->setCurrentItem(item);
  } else {
    navigation->setCurrentIndex({});  // no tree item to select (e.g. Profile)
  }
  applyPage(page, addToHistory);
}

void NavigationController::navigateToListStatus(anime::list::Status status) {
  const auto navigation = mainWindow_->navigation();
  if (const auto item = navigation->findListStatusItem(status)) {
    navigation->setCurrentItem(item);
  }
}

void NavigationController::applyPage(MainWindowPage page, bool addToHistory) {
  mainWindow_->initPage(page);
  mainWindow_->statusBarController()->clearMessage(StatusBarController::Source::Selection);
  mainWindow_->ui()->stackedWidget->setCurrentIndex(static_cast<int>(page));

  if (addToHistory) recordPageHistory(page);
}

void NavigationController::goBack() {
  if (pageHistoryIndex_ <= 0) return;

  navigateTo(pageHistory_.at(--pageHistoryIndex_), /*addToHistory=*/false);
  updateHistoryActions();
}

void NavigationController::goForward() {
  if (pageHistoryIndex_ >= pageHistory_.size() - 1) return;

  navigateTo(pageHistory_.at(++pageHistoryIndex_), /*addToHistory=*/false);
  updateHistoryActions();
}

void NavigationController::recordPageHistory(MainWindowPage page) {
  if (pageHistoryIndex_ >= 0 && pageHistory_.at(pageHistoryIndex_) == page) return;

  pageHistory_.removeAll(page);
  pageHistory_.append(page);
  pageHistoryIndex_ = pageHistory_.size() - 1;

  updateHistoryActions();
}

void NavigationController::updateHistoryActions() {
  mainWindow_->ui()->actionBack->setEnabled(pageHistoryIndex_ > 0);
  mainWindow_->ui()->actionForward->setEnabled(pageHistoryIndex_ < pageHistory_.size() - 1);
}

bool NavigationController::eventFilter(QObject* watched, QEvent* event) {
  const auto type = event->type();
  if (type != QEvent::MouseButtonPress && type != QEvent::MouseButtonRelease) {
    return QObject::eventFilter(watched, event);
  }

  const auto mouseEvent = static_cast<QMouseEvent*>(event);
  if (mouseEvent->button() != Qt::BackButton && mouseEvent->button() != Qt::ForwardButton) {
    return QObject::eventFilter(watched, event);
  }

  const auto widget = qobject_cast<QWidget*>(watched);
  if (!widget) {
    // Allow redispatching top-level window's event to the real widget.
    return QObject::eventFilter(watched, event);
  }

  if (type == QEvent::MouseButtonRelease && widget->window() == mainWindow_) {
    if (mouseEvent->button() == Qt::BackButton) {
      goBack();
    } else {
      goForward();
    }
  }

  // Consume so it never counts as a normal click (e.g. selecting a list item).
  return true;
}

}  // namespace gui
