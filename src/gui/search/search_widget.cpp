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

#include "search_widget.hpp"

#include <QToolBar>

#include "gui/search/search_list_widget.hpp"
#include "gui/utils/theme.hpp"
#include "ui_search_widget.h"

namespace gui {

SearchWidget::SearchWidget(QWidget* parent)
    : QWidget(parent), m_searchListWidget(new SearchListWidget(this)), ui_(new Ui::SearchWidget) {
  ui_->setupUi(this);

  // Year
  ui_->comboYear->clear();
  ui_->comboYear->addItem(tr("Any year"));
  ui_->comboYear->insertSeparator(1);
  for (int year = 2023 + 1; year >= 1940; --year) {
    ui_->comboYear->addItem(u"%1"_qs.arg(year));
  }

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
  }

  ui_->editFilter->hide();

  // List
  ui_->verticalLayout->addWidget(m_searchListWidget);
}

}  // namespace gui
