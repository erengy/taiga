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
    : QObject(mainWindow), m_mainWindow(mainWindow) {
  connect(mainWindow->navigation(), &NavigationWidget::currentPageChanged, this,
          [this](MainWindowPage page) { applyPage(page, /*addToHistory=*/true); });

  connect(mainWindow->ui()->actionBack, &QAction::triggered, this, &NavigationController::goBack);
  connect(mainWindow->ui()->actionForward, &QAction::triggered, this,
          &NavigationController::goForward);

  qApp->installEventFilter(this);
}

void NavigationController::navigateTo(MainWindowPage page, bool addToHistory) {
  const auto navigation = m_mainWindow->navigation();
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
  const auto navigation = m_mainWindow->navigation();
  if (const auto item = navigation->findListStatusItem(status)) {
    navigation->setCurrentItem(item);
  }
}

void NavigationController::applyPage(MainWindowPage page, bool addToHistory) {
  m_mainWindow->initPage(page);
  m_mainWindow->statusBarController()->clearMessage(StatusBarController::Source::Selection);
  m_mainWindow->ui()->stackedWidget->setCurrentIndex(static_cast<int>(page));

  if (addToHistory) recordPageHistory(page);
}

void NavigationController::goBack() {
  if (m_pageHistoryIndex <= 0) return;

  navigateTo(m_pageHistory.at(--m_pageHistoryIndex), /*addToHistory=*/false);
  updateHistoryActions();
}

void NavigationController::goForward() {
  if (m_pageHistoryIndex >= m_pageHistory.size() - 1) return;

  navigateTo(m_pageHistory.at(++m_pageHistoryIndex), /*addToHistory=*/false);
  updateHistoryActions();
}

void NavigationController::recordPageHistory(MainWindowPage page) {
  if (m_pageHistoryIndex >= 0 && m_pageHistory.at(m_pageHistoryIndex) == page) return;

  m_pageHistory.removeAll(page);
  m_pageHistory.append(page);
  m_pageHistoryIndex = m_pageHistory.size() - 1;

  updateHistoryActions();
}

void NavigationController::updateHistoryActions() {
  m_mainWindow->ui()->actionBack->setEnabled(m_pageHistoryIndex > 0);
  m_mainWindow->ui()->actionForward->setEnabled(m_pageHistoryIndex < m_pageHistory.size() - 1);
}

bool NavigationController::eventFilter(QObject* watched, QEvent* event) {
  if (event->type() == QEvent::MouseButtonRelease) {
    const auto mouseEvent = static_cast<QMouseEvent*>(event);

    if (mouseEvent->button() == Qt::BackButton || mouseEvent->button() == Qt::ForwardButton) {
      const auto widget = qobject_cast<QWidget*>(watched);
      if (widget && widget->window() == m_mainWindow) {
        if (mouseEvent->button() == Qt::BackButton) {
          goBack();
        } else {
          goForward();
        }
        return true;
      }
    }
  }

  return QObject::eventFilter(watched, event);
}

}  // namespace gui
