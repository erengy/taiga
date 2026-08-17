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

#include "search_widget.hpp"

#include <QActionGroup>
#include <QLineEdit>
#include <QToolBar>
#include <QToolButton>

#include "gui/common/anime_list_context.hpp"
#include "gui/common/anime_list_view.hpp"
#include "gui/common/anime_list_view_cards.hpp"
#include "gui/main/main_window.hpp"
#include "gui/models/anime_list_model.hpp"
#include "gui/models/anime_list_proxy_model.hpp"
#include "gui/utils/format.hpp"
#include "gui/utils/theme.hpp"
#include "media/anime.hpp"
#include "media/anime_season.hpp"
#include "sync/anilist/anilist.hpp"
#include "sync/kitsu/kitsu.hpp"
#include "sync/myanimelist/myanimelist.hpp"
#include "sync/service.hpp"
#include "taiga/session.hpp"

namespace {

std::optional<sync::SearchSort> toSearchSort(gui::AnimeListModel::Column column) {
  // clang-format off
  switch (column) {
    case gui::AnimeListModel::COLUMN_TITLE: return sync::SearchSort::Title;
    case gui::AnimeListModel::COLUMN_DURATION: return sync::SearchSort::Duration;
    case gui::AnimeListModel::COLUMN_AVERAGE: return sync::SearchSort::Score;
    case gui::AnimeListModel::COLUMN_TYPE: return sync::SearchSort::Type;
    case gui::AnimeListModel::COLUMN_SEASON: return sync::SearchSort::StartDate;
    default: return std::nullopt;
  }
  // clang-format on
}

}  // namespace

namespace gui {

SearchWidget::SearchWidget(QWidget* parent)
    : PageWidget(parent),
      m_model(new AnimeListModel(this)),
      m_proxyModel(new AnimeListProxyModel(this)),
      m_comboYear(new ComboBox(this)),
      m_comboSeason(new ComboBox(this)),
      m_comboType(new ComboBox(this)),
      m_comboStatus(new ComboBox(this)),
      m_comboListStatus(new ComboBox(this)),
      m_sortMenu(new QMenu(this)),
      m_viewMenu(new QMenu(this)) {
  m_proxyModel->sort(taiga::session.searchListSortColumn(), taiga::session.searchListSortOrder());
  m_proxyModel->setFilters(taiga::session.searchListFilters());

  static const auto filterValue = [](QComboBox* combo, int index) {
    return index > -1 ? std::optional<int>{combo->itemData(index).toInt()} : std::nullopt;
  };

  auto filtersLayout = new QHBoxLayout();
  filtersLayout->setSpacing(4);
  m_toolbarLayout->insertLayout(0, filtersLayout);

  // Year
  {
    m_comboYear->setPlaceholderText("Year");
    for (int year = QDate::currentDate().year() + 1; year >= 1940; --year) {
      m_comboYear->addItem(QString::number(year), year);
    }
    if (m_proxyModel->filters().year) {
      m_comboYear->setCurrentText(QString::number(*m_proxyModel->filters().year));
    }
    connect(m_comboYear, &QComboBox::currentIndexChanged, this, [this](int index) {
      m_proxyModel->setYearFilter(filterValue(m_comboYear, index));
      performSearch();
    });
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
      performSearch();
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
    connect(m_comboType, &QComboBox::currentIndexChanged, this, [this](int index) {
      m_proxyModel->setTypeFilter(filterValue(m_comboType, index));
      performSearch();
    });
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
      performSearch();
    });
    filtersLayout->addWidget(m_comboStatus);
  }

  // List status
  {
    m_comboListStatus->setPlaceholderText("List status");
    for (const auto status : anime::list::kStatuses) {
      m_comboListStatus->addItem(formatListStatus(status), static_cast<int>(status));
    }
    if (m_proxyModel->filters().listStatus.status) {
      m_comboListStatus->setCurrentText(formatListStatus(
          static_cast<anime::list::Status>(*m_proxyModel->filters().listStatus.status)));
    }
    connect(m_comboListStatus, &QComboBox::currentIndexChanged, this, [this](int index) {
      m_proxyModel->setListStatusFilter({
          .status = filterValue(m_comboListStatus, index),
      });
    });
    filtersLayout->addWidget(m_comboListStatus);
  }

  // Toolbar
  {
    const auto actionSort = new QAction(theme.getIcon("sort"), tr("Sort"), this);
    const auto actionView = new QAction(theme.getIcon("grid_view"), tr("View"), this);
    const auto actionMore = new QAction(theme.getIcon("more_horiz"), tr("More"), this);
    m_toolbar->addAction(actionSort);
    m_toolbar->addAction(actionView);
    m_toolbar->addAction(actionMore);

    const auto sortButton = static_cast<QToolButton*>(m_toolbar->widgetForAction(actionSort));
    sortButton->setPopupMode(QToolButton::InstantPopup);
    sortButton->setMenu(m_sortMenu);

    const auto viewButton = static_cast<QToolButton*>(m_toolbar->widgetForAction(actionView));
    viewButton->setPopupMode(QToolButton::InstantPopup);
    viewButton->setMenu(m_viewMenu);
  }

  connect(m_sortMenu, &QMenu::aboutToShow, this, &SearchWidget::initSortMenu);
  connect(m_viewMenu, &QMenu::aboutToShow, this, &SearchWidget::initViewMenu);

  // List
  setViewMode(taiga::session.searchListViewMode());

  // Search
  connect(mainWindow()->searchBox(), &QLineEdit::returnPressed, this,
          [this]() { performSearch(); });

  const QList<sync::Service*> services{
      sync::anilist::Service::instance(),
      sync::kitsu::Service::instance(),
      sync::myanimelist::Service::instance(),
  };
  for (auto* service : services) {
    connect(service, &sync::Service::searchCompleted, this,
            [this](const sync::SearchParams& params, const QList<int>& ids) {
              if (params != currentSearchParams()) return;
              m_model->addIds(ids);
            });
  }
}

void SearchWidget::saveState() {
  taiga::session.setSearchListFilters(m_proxyModel->filters());
  taiga::session.setSearchListSortColumn(m_proxyModel->sortColumn());
  taiga::session.setSearchListSortOrder(m_proxyModel->sortOrder());
  taiga::session.setSearchListViewMode(m_viewMode);
}

void SearchWidget::initSortMenu() {
  using Qt::SortOrder::AscendingOrder;
  using Qt::SortOrder::DescendingOrder;

  static const QList<QPair<AnimeListModel::Column, Qt::SortOrder>> items{
      {AnimeListModel::COLUMN_TITLE, AscendingOrder},
      {AnimeListModel::COLUMN_DURATION, DescendingOrder},
      {AnimeListModel::COLUMN_AVERAGE, DescendingOrder},
      {AnimeListModel::COLUMN_TYPE, AscendingOrder},
      {AnimeListModel::COLUMN_SEASON, DescendingOrder},
      {AnimeListModel::COLUMN_STARTED, DescendingOrder},
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

      const auto params = currentSearchParams();
      if (!(params.season && params.year)) {
        performSearch();
      }
    });

    action->setCheckable(true);
    action->setChecked(column == m_proxyModel->sortColumn());
    actionGroup->addAction(action);
  }
}

void SearchWidget::initViewMenu() {
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

void SearchWidget::setViewMode(ListViewMode mode) {
  if (m_listView) {
    layout()->removeWidget(m_listView);
    m_listView->deleteLater();
    m_listView = nullptr;
  }
  if (m_listViewCards) {
    layout()->removeWidget(m_listViewCards);
    m_listViewCards->deleteLater();
    m_listViewCards = nullptr;
  }

  m_viewMode = mode;

  switch (mode) {
    case ListViewMode::List:
      m_listView = new ListView(this, m_model, m_proxyModel, AnimeListContext::Search);
      layout()->addWidget(m_listView);
      m_listView->show();
      break;

    case ListViewMode::Cards:
      m_listViewCards = new ListViewCards(this, m_model, m_proxyModel, AnimeListContext::Search);
      layout()->addWidget(m_listViewCards);
      m_listViewCards->show();
      break;
  }
}

sync::SearchParams SearchWidget::currentSearchParams() const {
  const auto& filters = m_proxyModel->filters();

  return {
      .text = mainWindow()->searchBox()->text(),
      .year = filters.year,
      .season = filters.season ? std::make_optional(static_cast<anime::SeasonName>(*filters.season))
                               : std::nullopt,
      .type =
          filters.type ? std::make_optional(static_cast<anime::Type>(*filters.type)) : std::nullopt,
      .status = filters.status ? std::make_optional(static_cast<anime::Status>(*filters.status))
                               : std::nullopt,
      .sort = toSearchSort(static_cast<AnimeListModel::Column>(m_proxyModel->sortColumn())),
      .sortOrder = m_proxyModel->sortOrder(),
  };
}

void SearchWidget::performSearch() {
  if (!isVisible()) return;

  const auto params = currentSearchParams();

  if (params.text.isEmpty() && !params.year && !params.season && !params.type && !params.status) {
    return;
  }

  sync::search(params);
}

}  // namespace gui
