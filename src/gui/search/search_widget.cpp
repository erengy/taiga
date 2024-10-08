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

#include "gui/common/anime_list_view_cards.hpp"
#include "gui/models/anime_list_model.hpp"
#include "gui/models/anime_list_proxy_model.hpp"
#include "gui/utils/format.hpp"
#include "gui/utils/theme.hpp"
#include "media/anime.hpp"
#include "media/anime_season.hpp"
#include "ui_search_widget.h"

namespace gui {

SearchWidget::SearchWidget(QWidget* parent, MainWindow* mainWindow)
    : QWidget(parent),
      m_model(new AnimeListModel(this)),
      m_proxyModel(new AnimeListProxyModel(this)),
      m_listViewCards(new ListViewCards(this, m_model, m_proxyModel, mainWindow)),
      m_mainWindow(mainWindow),
      ui_(new Ui::SearchWidget) {
  ui_->setupUi(this);

  // Year
  {
    ui_->comboYear->clear();
    ui_->comboYear->addItem(tr("Unknown"), 0);
    ui_->comboYear->insertSeparator(1);
    for (int year = 2023 + 1; year >= 1940; --year) {
      ui_->comboYear->addItem(u"%1"_qs.arg(year), year);
    }
    connect(ui_->comboYear, &QComboBox::currentIndexChanged, this, [this](int index) {
      const int year = ui_->comboYear->itemData(index).toInt();
      m_proxyModel->setYearFilter(year);
    });
  }

  // Season
  {
    ui_->comboSeason->clear();
    const auto seasons = {
        anime::SeasonName::Winter,
        anime::SeasonName::Spring,
        anime::SeasonName::Summer,
        anime::SeasonName::Fall,
    };
    for (const auto season : seasons) {
      ui_->comboSeason->addItem(formatSeasonName(season), static_cast<int>(season));
    }
    connect(ui_->comboSeason, &QComboBox::currentIndexChanged, this, [this](int index) {
      const int season = ui_->comboSeason->itemData(index).toInt();
      m_proxyModel->setSeasonFilter(season);
    });
  }

  // Type
  {
    ui_->comboType->clear();
    for (const auto type : anime::kTypes) {
      ui_->comboType->addItem(formatType(type), static_cast<int>(type));
    }
    connect(ui_->comboType, &QComboBox::currentIndexChanged, this, [this](int index) {
      const int type = ui_->comboType->itemData(index).toInt();
      m_proxyModel->setTypeFilter(type);
    });
  }

  // Status
  {
    ui_->comboStatus->clear();
    for (const auto status : anime::kStatuses) {
      ui_->comboStatus->addItem(formatStatus(status), static_cast<int>(status));
    }
    connect(ui_->comboStatus, &QComboBox::currentIndexChanged, this, [this](int index) {
      const int status = ui_->comboStatus->itemData(index).toInt();
      m_proxyModel->setStatusFilter(status);
    });
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
  ui_->verticalLayout->addWidget(m_listViewCards);
}

}  // namespace gui
