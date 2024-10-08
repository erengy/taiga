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

#include <QListView>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>

#include "gui/common/anime_list_view.hpp"
#include "gui/common/anime_list_view_cards.hpp"
#include "gui/models/anime_list_model.hpp"
#include "gui/models/anime_list_proxy_model.hpp"
#include "gui/utils/theme.hpp"
#include "ui_list_widget.h"

namespace {

struct SortMenuItem {
  QString title;
  gui::AnimeListModel::Column column;
  Qt::SortOrder order;
};

using Qt::SortOrder::AscendingOrder;
using Qt::SortOrder::DescendingOrder;

}  // namespace

namespace gui {

ListWidget::ListWidget(QWidget* parent, MainWindow* mainWindow)
    : QWidget(parent),
      m_model(new AnimeListModel(this)),
      m_proxyModel(new AnimeListProxyModel(this)),
      m_listView(new ListView(this, m_model, m_proxyModel, mainWindow)),
      m_listViewCards(new ListViewCards(this, m_model, m_proxyModel, mainWindow)),
      m_mainWindow(mainWindow),
      ui_(new Ui::ListWidget) {
  ui_->setupUi(this);

  ui_->filterButton->hide();

  // Toolbar
  {
    auto toolbar = new QToolBar(this);
    toolbar->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
    toolbar->setIconSize({18, 18});

    toolbar->addAction("TV");
    toolbar->addAction("Finished airing");
    toolbar->addAction("Average > 80%");

    {
      auto button = new QToolButton(this);
      button->setToolButtonStyle(Qt::ToolButtonIconOnly);
      button->setIcon(theme.getIcon("add_box"));
      button->setText("Add filter");
      toolbar->addWidget(button);
    }

    ui_->toolbarLayout->insertWidget(1, toolbar);
  }

  ui_->editFilter->hide();

  // Toolbar
  {
    auto toolbar = new QToolBar(this);
    toolbar->setIconSize({18, 18});
    ui_->actionSort->setIcon(theme.getIcon("sort"));
    ui_->actionView->setIcon(theme.getIcon("grid_view"));
    ui_->actionMore->setIcon(theme.getIcon("more_horiz"));
    toolbar->addAction(ui_->actionSort);
    toolbar->addAction(ui_->actionView);
    toolbar->addAction(ui_->actionMore);
    ui_->toolbarLayout->addWidget(toolbar);

    {
      const auto button = static_cast<QToolButton*>(toolbar->widgetForAction(ui_->actionSort));
      button->setPopupMode(QToolButton::InstantPopup);
      button->setMenu([this]() {
        auto menu = new QMenu(this);
        static const QList<SortMenuItem> sortMenuItems = {
            {tr("Title"), gui::AnimeListModel::COLUMN_TITLE, AscendingOrder},
            {tr("Progress"), gui::AnimeListModel::COLUMN_PROGRESS, DescendingOrder},
            {tr("Score"), gui::AnimeListModel::COLUMN_SCORE, DescendingOrder},
            {tr("Average"), gui::AnimeListModel::COLUMN_AVERAGE, DescendingOrder},
            {tr("Type"), gui::AnimeListModel::COLUMN_TYPE, AscendingOrder},
            {tr("Season"), gui::AnimeListModel::COLUMN_SEASON, DescendingOrder},
            {tr("Started"), gui::AnimeListModel::COLUMN_STARTED, DescendingOrder},
            {tr("Completed"), gui::AnimeListModel::COLUMN_COMPLETED, DescendingOrder},
            {tr("Last updated"), gui::AnimeListModel::COLUMN_LAST_UPDATED, DescendingOrder},
        };
        for (const auto& item : sortMenuItems) {
          menu->addAction(item.title, this,
                          [this, item]() { m_listView->sortByColumn(item.column, item.order); });
        }
        return menu;
      }());
    }

    {
      const auto button = static_cast<QToolButton*>(toolbar->widgetForAction(ui_->actionView));
      button->setPopupMode(QToolButton::InstantPopup);
      button->setMenu([this]() {
        auto menu = new QMenu(this);
        menu->addAction("List", this, [this]() { setViewMode(ListViewMode::List); });
        menu->addAction("Cards", this, [this]() { setViewMode(ListViewMode::Cards); });
        return menu;
      }());
    }
  }

  // List
  m_proxyModel->setListStatusFilter(static_cast<int>(anime::list::Status::Watching));
  setViewMode(ListViewMode::List);
}

ListViewMode ListWidget::viewMode() const {
  return m_viewMode;
}

void ListWidget::setViewMode(ListViewMode mode) {
  m_viewMode = mode;

  m_listView->hide();
  m_listViewCards->hide();

  ui_->verticalLayout->removeWidget(m_listView);
  ui_->verticalLayout->removeWidget(m_listViewCards);

  switch (mode) {
    case ListViewMode::List:
      ui_->verticalLayout->addWidget(m_listView);
      m_listView->show();
      break;
    case ListViewMode::Cards:
      ui_->verticalLayout->addWidget(m_listViewCards);
      m_listViewCards->show();
      break;
  }
}

}  // namespace gui
