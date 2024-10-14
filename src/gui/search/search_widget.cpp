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

namespace gui {

SearchWidget::SearchWidget(QWidget* parent, MainWindow* mainWindow)
    : PageWidget(parent),
      m_model(new AnimeListModel(this)),
      m_proxyModel(new AnimeListProxyModel(this)),
      m_listViewCards(new ListViewCards(this, m_model, m_proxyModel, mainWindow)),
      m_mainWindow(mainWindow),
      m_comboYear(new ComboBox(this)),
      m_comboSeason(new ComboBox(this)),
      m_comboType(new ComboBox(this)),
      m_comboStatus(new ComboBox(this)) {
  // @TODO: Use settings from the previous session
  m_proxyModel->sort(AnimeListModel::COLUMN_AVERAGE, Qt::SortOrder::DescendingOrder);
  m_proxyModel->setYearFilter(QDate::currentDate().year());
  m_proxyModel->setSeasonFilter(
      static_cast<int>(anime::Season{QDate::currentDate().toStdSysDays()}.name));
  m_proxyModel->setTypeFilter(static_cast<int>(anime::Type::Tv));

  static const auto filterValue = [](QComboBox* combo, int index) {
    return index > -1 ? std::optional<int>{combo->itemData(index).toInt()} : std::nullopt;
  };

  auto filtersLayout = new QHBoxLayout(this);
  filtersLayout->setSpacing(4);
  m_toolbarLayout->insertLayout(0, filtersLayout);

  // Year
  {
    m_comboYear->setPlaceholderText("Year");
    for (int year = QDate::currentDate().year() + 1; year >= 1940; --year) {
      m_comboYear->addItem(u"%1"_qs.arg(year), year);
    }
    if (m_proxyModel->filters().year) {
      m_comboYear->setCurrentText(QString::number(*m_proxyModel->filters().year));
    }
    connect(m_comboYear, &QComboBox::currentIndexChanged, this,
            [this](int index) { m_proxyModel->setYearFilter(filterValue(m_comboYear, index)); });
    filtersLayout->addWidget(m_comboYear);
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
    if (m_proxyModel->filters().season) {
      m_comboSeason->setCurrentText(
          formatSeasonName(static_cast<anime::SeasonName>(*m_proxyModel->filters().season)));
    }
    connect(m_comboSeason, &QComboBox::currentIndexChanged, this, [this](int index) {
      m_proxyModel->setSeasonFilter(filterValue(m_comboSeason, index));
    });
    filtersLayout->addWidget(m_comboSeason);
  }

  // Type
  {
    m_comboType->setPlaceholderText("Type");
    for (const auto type : anime::kTypes) {
      m_comboType->addItem(formatType(type), static_cast<int>(type));
    }
    if (m_proxyModel->filters().type) {
      m_comboType->setCurrentText(
          formatType(static_cast<anime::Type>(*m_proxyModel->filters().type)));
    }
    connect(m_comboType, &QComboBox::currentIndexChanged, this,
            [this](int index) { m_proxyModel->setTypeFilter(filterValue(m_comboType, index)); });
    filtersLayout->addWidget(m_comboType);
  }

  // Status
  {
    m_comboStatus->setPlaceholderText("Status");
    for (const auto status : anime::kStatuses) {
      m_comboStatus->addItem(formatStatus(status), static_cast<int>(status));
    }
    if (m_proxyModel->filters().status) {
      m_comboStatus->setCurrentText(
          formatStatus(static_cast<anime::Status>(*m_proxyModel->filters().status)));
    }
    connect(m_comboStatus, &QComboBox::currentIndexChanged, this, [this](int index) {
      m_proxyModel->setStatusFilter(filterValue(m_comboStatus, index));
    });
    filtersLayout->addWidget(m_comboStatus);
  }

  // Toolbar
  {
    const auto actionSort = new QAction(theme.getIcon("sort"), tr("Sort"), this);
    const auto actionView = new QAction(theme.getIcon("grid_view"), tr("View"), this);
    const auto actionMore = new QAction(theme.getIcon("more_horiz"), tr("More"), this);
    m_toolbar->addAction(actionSort);
    m_toolbar->addAction(actionView);
    m_toolbar->addAction(actionMore);
  }

  // List
  layout()->addWidget(m_listViewCards);
}

}  // namespace gui
