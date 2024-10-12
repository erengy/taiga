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

#include "list_widget.hpp"

#include <QActionGroup>
#include <QListView>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>

#include "gui/common/anime_list_view.hpp"
#include "gui/common/anime_list_view_cards.hpp"
#include "gui/main/main_window.hpp"
#include "gui/main/navigation_widget.hpp"
#include "gui/models/anime_list_model.hpp"
#include "gui/models/anime_list_proxy_model.hpp"
#include "gui/utils/theme.hpp"
#include "ui_list_widget.h"

namespace gui {

ListWidget::ListWidget(QWidget* parent, MainWindow* mainWindow)
    : QWidget(parent),
      m_model(new AnimeListModel(this)),
      m_proxyModel(new AnimeListProxyModel(this)),
      m_mainWindow(mainWindow),
      m_sortMenu(new QMenu(this)),
      m_viewMenu(new QMenu(this)),
      ui_(new Ui::ListWidget) {
  ui_->setupUi(this);

  // @TODO: Use settings from the previous session
  m_proxyModel->sort(AnimeListModel::COLUMN_LAST_UPDATED, Qt::SortOrder::DescendingOrder);

  initToolbar();
  setViewMode(ListViewMode::List);

  connect(m_sortMenu, &QMenu::aboutToShow, this, &ListWidget::initSortMenu);
  connect(m_viewMenu, &QMenu::aboutToShow, this, &ListWidget::initViewMenu);

  connect(mainWindow->navigation(), &NavigationWidget::currentListStatusChanged, this,
          [this](anime::list::Status status) {
            m_proxyModel->setListStatusFilter({
                .status = static_cast<int>(status),
                .anyStatus = !static_cast<int>(status),
            });
          });
}

ListViewMode ListWidget::viewMode() const {
  return m_viewMode;
}

void ListWidget::setViewMode(ListViewMode mode) {
  if (m_listView) {
    ui_->verticalLayout->removeWidget(m_listView);
    m_listView->deleteLater();
    m_listView = nullptr;
  };
  if (m_listViewCards) {
    ui_->verticalLayout->removeWidget(m_listViewCards);
    m_listViewCards->deleteLater();
    m_listViewCards = nullptr;
  };

  m_viewMode = mode;

  switch (mode) {
    case ListViewMode::List:
      m_listView = new ListView(this, m_model, m_proxyModel, m_mainWindow);
      ui_->verticalLayout->addWidget(m_listView);
      m_listView->show();
      break;

    case ListViewMode::Cards:
      m_listViewCards = new ListViewCards(this, m_model, m_proxyModel, m_mainWindow);
      ui_->verticalLayout->addWidget(m_listViewCards);
      m_listViewCards->show();
      break;
  }
}

void ListWidget::initToolbar() {
  const auto toolbar = new QToolBar(this);

  toolbar->setIconSize({18, 18});

  ui_->actionSort->setIcon(theme.getIcon("sort"));
  ui_->actionView->setIcon(theme.getIcon("grid_view"));
  ui_->actionMore->setIcon(theme.getIcon("more_horiz"));

  toolbar->addAction(ui_->actionSort);
  toolbar->addAction(ui_->actionView);
  toolbar->addAction(ui_->actionMore);

  ui_->toolbarLayout->addWidget(toolbar);

  const auto sortButton = static_cast<QToolButton*>(toolbar->widgetForAction(ui_->actionSort));
  sortButton->setPopupMode(QToolButton::InstantPopup);
  sortButton->setMenu(m_sortMenu);

  const auto viewButton = static_cast<QToolButton*>(toolbar->widgetForAction(ui_->actionView));
  viewButton->setPopupMode(QToolButton::InstantPopup);
  viewButton->setMenu(m_viewMenu);
}

void ListWidget::initSortMenu() {
  using Qt::SortOrder::AscendingOrder;
  using Qt::SortOrder::DescendingOrder;

  static const QList<QPair<AnimeListModel::Column, Qt::SortOrder>> items{
      {AnimeListModel::COLUMN_TITLE, AscendingOrder},
      {AnimeListModel::COLUMN_PROGRESS, DescendingOrder},
      {AnimeListModel::COLUMN_DURATION, DescendingOrder},
      {AnimeListModel::COLUMN_REWATCHES, DescendingOrder},
      {AnimeListModel::COLUMN_SCORE, DescendingOrder},
      {AnimeListModel::COLUMN_AVERAGE, DescendingOrder},
      {AnimeListModel::COLUMN_TYPE, AscendingOrder},
      {AnimeListModel::COLUMN_SEASON, DescendingOrder},
      {AnimeListModel::COLUMN_STARTED, DescendingOrder},
      {AnimeListModel::COLUMN_COMPLETED, DescendingOrder},
      {AnimeListModel::COLUMN_LAST_UPDATED, DescendingOrder},
      {AnimeListModel::COLUMN_NOTES, AscendingOrder},
  };

  const auto actionGroup = new QActionGroup(this);

  m_sortMenu->clear();

  for (const auto& [column, order] : items) {
    const auto headerData =
        m_model->headerData(column, Qt::Orientation::Horizontal, Qt::DisplayRole);

    const auto action = m_sortMenu->addAction(headerData.toString(), this, [this, column, order]() {
      if (m_listView) {
        // Sorting the proxy model doesn't update the sort indicator on the header.
        m_listView->sortByColumn(column, order);
      } else {
        m_proxyModel->sort(column, order);
      }
    });

    action->setCheckable(true);
    action->setChecked(column == m_proxyModel->sortColumn());
    actionGroup->addAction(action);
  }
}

void ListWidget::initViewMenu() {
  static const QList<QPair<QString, ListViewMode>> items{
      {"List", ListViewMode::List},
      {"Cards", ListViewMode::Cards},
  };

  const auto actionGroup = new QActionGroup(this);

  m_viewMenu->clear();

  for (const auto& [text, mode] : items) {
    const auto action = m_viewMenu->addAction(text, this, [this, mode]() { setViewMode(mode); });
    action->setCheckable(true);
    action->setChecked(mode == m_viewMode);
    actionGroup->addAction(action);
  }
}

}  // namespace gui
