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
      m_comboYear(new ComboBox(this)),
      m_comboSeason(new ComboBox(this)),
      m_comboType(new ComboBox(this)),
      m_comboStatus(new ComboBox(this)),
      ui_(new Ui::SearchWidget) {
  ui_->setupUi(this);

  static const auto filterValue = [](QComboBox* combo, int index) {
    return index > -1 ? std::optional<int>{combo->itemData(index).toInt()} : std::nullopt;
  };

  // Year
  {
    m_comboYear->setPlaceholderText("Year");
    for (int year = QDate::currentDate().year() + 1; year >= 1940; --year) {
      m_comboYear->addItem(u"%1"_qs.arg(year), year);
    }
    connect(m_comboYear, &QComboBox::currentIndexChanged, this,
            [this](int index) { m_proxyModel->setYearFilter(filterValue(m_comboYear, index)); });
    ui_->toolbarLayout->insertWidget(0, m_comboYear);
  }

  // Season
  {
    m_comboSeason->setPlaceholderText("Season");
    const auto seasons = {
        anime::SeasonName::Winter,
        anime::SeasonName::Spring,
        anime::SeasonName::Summer,
        anime::SeasonName::Fall,
    };
    for (const auto season : seasons) {
      m_comboSeason->addItem(formatSeasonName(season), static_cast<int>(season));
    }
    connect(m_comboSeason, &QComboBox::currentIndexChanged, this, [this](int index) {
      m_proxyModel->setSeasonFilter(filterValue(m_comboSeason, index));
    });
    ui_->toolbarLayout->insertWidget(1, m_comboSeason);
  }

  // Type
  {
    m_comboType->setPlaceholderText("Type");
    for (const auto type : anime::kTypes) {
      m_comboType->addItem(formatType(type), static_cast<int>(type));
    }
    connect(m_comboType, &QComboBox::currentIndexChanged, this,
            [this](int index) { m_proxyModel->setTypeFilter(filterValue(m_comboType, index)); });
    ui_->toolbarLayout->insertWidget(2, m_comboType);
  }

  // Status
  {
    m_comboStatus->setPlaceholderText("Status");
    for (const auto status : anime::kStatuses) {
      m_comboStatus->addItem(formatStatus(status), static_cast<int>(status));
    }
    connect(m_comboStatus, &QComboBox::currentIndexChanged, this, [this](int index) {
      m_proxyModel->setStatusFilter(filterValue(m_comboStatus, index));
    });
    ui_->toolbarLayout->insertWidget(3, m_comboStatus);
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

  // List
  ui_->verticalLayout->addWidget(m_listViewCards);
}

}  // namespace gui
