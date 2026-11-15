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
#include "taiga/settings.hpp"

namespace {

std::optional<sync::SearchSort> toSearchSort(gui::AnimeListModel::Column column) {
  // clang-format off
  switch (column) {
    case gui::AnimeListModel::ColumnTitle: return sync::SearchSort::Title;
    case gui::AnimeListModel::ColumnDuration: return sync::SearchSort::Duration;
    case gui::AnimeListModel::ColumnAverage: return sync::SearchSort::Score;
    case gui::AnimeListModel::ColumnType: return sync::SearchSort::Type;
    case gui::AnimeListModel::ColumnSeason: return sync::SearchSort::StartDate;
    default: return std::nullopt;
  }
  // clang-format on
}

}  // namespace

namespace gui {

SearchWidget::SearchWidget(QWidget* parent)
    : PageWidget(parent),
      model_(new AnimeListModel(this)),
      proxyModel_(new AnimeListProxyModel(this)),
      comboYear_(new ComboBox(this)),
      comboSeason_(new ComboBox(this)),
      comboType_(new ComboBox(this)),
      comboStatus_(new ComboBox(this)),
      comboListStatus_(new ComboBox(this)),
      sortMenu_(new QMenu(this)),
      viewMenu_(new QMenu(this)),
      moreMenu_(new QMenu(this)) {
  proxyModel_->setSecondarySort(taiga::session.searchListSecondarySortColumn(),
                                taiga::session.searchListSecondarySortOrder());
  proxyModel_->sort(taiga::session.searchListSortColumn(), taiga::session.searchListSortOrder());
  proxyModel_->setFilters(taiga::session.searchListFilters());
  proxyModel_->setNsfwHidden(taiga::settings()->appHideNsfw());

  connect(taiga::settings(), &taiga::Settings::changed, this, [this](const QSet<QString>& keys) {
    if (keys.contains(taiga::Settings::Key::kAppHideNsfw)) {
      proxyModel_->setNsfwHidden(taiga::settings()->appHideNsfw());
    }
  });

  static const auto filterValue = [](QComboBox* combo, int index) {
    return index > -1 ? std::optional<int>{combo->itemData(index).toInt()} : std::nullopt;
  };

  auto filtersLayout = new QHBoxLayout();
  filtersLayout->setSpacing(4);
  toolbarLayout_->insertLayout(0, filtersLayout);

  // Year
  {
    comboYear_->setPlaceholderText("Year");
    for (int year = QDate::currentDate().year() + 1; year >= 1940; --year) {
      comboYear_->addItem(QString::number(year), year);
    }
    if (proxyModel_->filters().year) {
      comboYear_->setCurrentText(QString::number(*proxyModel_->filters().year));
    }
    connect(comboYear_, &QComboBox::currentIndexChanged, this, [this](int index) {
      proxyModel_->setYearFilter(filterValue(comboYear_, index));
      performSearch();
    });
    filtersLayout->addWidget(comboYear_);
  }

  // Season
  {
    comboSeason_->setPlaceholderText("Season");
    const auto seasons = {
        anime::SeasonName::Winter,
        anime::SeasonName::Spring,
        anime::SeasonName::Summer,
        anime::SeasonName::Fall,
    };
    for (const auto season : seasons) {
      comboSeason_->addItem(formatSeasonName(season), static_cast<int>(season));
    }
    if (proxyModel_->filters().season) {
      comboSeason_->setCurrentText(
          formatSeasonName(static_cast<anime::SeasonName>(*proxyModel_->filters().season)));
    }
    connect(comboSeason_, &QComboBox::currentIndexChanged, this, [this](int index) {
      proxyModel_->setSeasonFilter(filterValue(comboSeason_, index));
      performSearch();
    });
    filtersLayout->addWidget(comboSeason_);
  }

  // Type
  {
    comboType_->setPlaceholderText("Type");
    for (const auto type : anime::kTypes) {
      comboType_->addItem(formatType(type), static_cast<int>(type));
    }
    if (proxyModel_->filters().type) {
      comboType_->setCurrentText(
          formatType(static_cast<anime::Type>(*proxyModel_->filters().type)));
    }
    connect(comboType_, &QComboBox::currentIndexChanged, this, [this](int index) {
      proxyModel_->setTypeFilter(filterValue(comboType_, index));
      performSearch();
    });
    filtersLayout->addWidget(comboType_);
  }

  // Status
  {
    comboStatus_->setPlaceholderText("Status");
    for (const auto status : anime::kStatuses) {
      comboStatus_->addItem(formatStatus(status), static_cast<int>(status));
    }
    if (proxyModel_->filters().status) {
      comboStatus_->setCurrentText(
          formatStatus(static_cast<anime::Status>(*proxyModel_->filters().status)));
    }
    connect(comboStatus_, &QComboBox::currentIndexChanged, this, [this](int index) {
      proxyModel_->setStatusFilter(filterValue(comboStatus_, index));
      performSearch();
    });
    filtersLayout->addWidget(comboStatus_);
  }

  // List status
  {
    comboListStatus_->setPlaceholderText("List status");
    for (const auto status : anime::list::kStatuses) {
      comboListStatus_->addItem(formatListStatus(status), static_cast<int>(status));
    }
    if (proxyModel_->filters().listStatus.status) {
      comboListStatus_->setCurrentText(formatListStatus(
          static_cast<anime::list::Status>(*proxyModel_->filters().listStatus.status)));
    }
    connect(comboListStatus_, &QComboBox::currentIndexChanged, this, [this](int index) {
      proxyModel_->setListStatusFilter({
          .status = filterValue(comboListStatus_, index),
      });
    });
    filtersLayout->addWidget(comboListStatus_);
  }

  // Toolbar
  {
    const auto actionSort = new QAction(theme()->getIcon("sort"), tr("Sort"), this);
    const auto actionView = new QAction(theme()->getIcon("grid_view"), tr("View"), this);
    const auto actionMore = new QAction(theme()->getIcon("more_horiz"), tr("More"), this);
    toolbar_->addAction(actionSort);
    toolbar_->addAction(actionView);
    toolbar_->addAction(actionMore);

    const auto sortButton = static_cast<QToolButton*>(toolbar_->widgetForAction(actionSort));
    sortButton->setPopupMode(QToolButton::InstantPopup);
    sortButton->setMenu(sortMenu_);

    const auto viewButton = static_cast<QToolButton*>(toolbar_->widgetForAction(actionView));
    viewButton->setPopupMode(QToolButton::InstantPopup);
    viewButton->setMenu(viewMenu_);

    const auto moreButton = static_cast<QToolButton*>(toolbar_->widgetForAction(actionMore));
    moreButton->setPopupMode(QToolButton::InstantPopup);
    moreButton->setMenu(moreMenu_);
  }

  connect(sortMenu_, &QMenu::aboutToShow, this, &SearchWidget::initSortMenu);
  connect(viewMenu_, &QMenu::aboutToShow, this, &SearchWidget::initViewMenu);
  connect(moreMenu_, &QMenu::aboutToShow, this, &SearchWidget::initMoreMenu);

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
              model_->addIds(ids);
            });
  }
}

void SearchWidget::saveState() {
  taiga::session.setSearchListFilters(proxyModel_->filters());
  taiga::session.setSearchListSortColumn(proxyModel_->sortColumn());
  taiga::session.setSearchListSortOrder(proxyModel_->sortOrder());
  taiga::session.setSearchListSecondarySortColumn(proxyModel_->secondarySortColumn());
  taiga::session.setSearchListSecondarySortOrder(proxyModel_->secondarySortOrder());
  taiga::session.setSearchListViewMode(viewMode_);
}

void SearchWidget::initSortMenu() {
  using Qt::SortOrder::AscendingOrder;
  using Qt::SortOrder::DescendingOrder;

  static const QList<QPair<AnimeListModel::Column, Qt::SortOrder>> items{
      {AnimeListModel::ColumnTitle, AscendingOrder},
      {AnimeListModel::ColumnDuration, DescendingOrder},
      {AnimeListModel::ColumnAverage, DescendingOrder},
      {AnimeListModel::ColumnType, AscendingOrder},
      {AnimeListModel::ColumnSeason, DescendingOrder},
      {AnimeListModel::ColumnStarted, DescendingOrder},
  };

  const auto actionGroup = new QActionGroup(this);

  sortMenu_->clear();

  for (const auto& [column, order] : items) {
    const auto headerData =
        model_->headerData(column, Qt::Orientation::Horizontal, Qt::DisplayRole);

    const auto action = sortMenu_->addAction(headerData.toString(), this, [this, column, order]() {
      if (listView_) {
        // Sorting the proxy model doesn't update the sort indicator on the header.
        listView_->sortByColumn(column, order);
      } else {
        proxyModel_->sort(column, order);
      }

      const auto params = currentSearchParams();
      if (!(params.season && params.year)) {
        performSearch();
      }
    });

    action->setCheckable(true);
    action->setChecked(column == proxyModel_->sortColumn());
    actionGroup->addAction(action);
  }
}

void SearchWidget::initViewMenu() {
  static const QList<QPair<QString, ListViewMode>> items{
      {"List", ListViewMode::List},
      {"Cards", ListViewMode::Cards},
  };

  const auto actionGroup = new QActionGroup(this);

  viewMenu_->clear();

  for (const auto& [text, mode] : items) {
    const auto action = viewMenu_->addAction(text, this, [this, mode]() { setViewMode(mode); });
    action->setCheckable(true);
    action->setChecked(mode == viewMode_);
    actionGroup->addAction(action);
  }
}

void SearchWidget::initMoreMenu() {
  moreMenu_->clear();

  const auto actionHideNsfw =
      moreMenu_->addAction(tr("Hide adult content"), this,
                           [](bool checked) { taiga::settings()->setAppHideNsfw(checked); });
  actionHideNsfw->setCheckable(true);
  actionHideNsfw->setChecked(taiga::settings()->appHideNsfw());
}

void SearchWidget::setViewMode(ListViewMode mode) {
  if (listView_) {
    layout()->removeWidget(listView_);
    listView_->deleteLater();
    listView_ = nullptr;
  }
  if (listViewCards_) {
    layout()->removeWidget(listViewCards_);
    listViewCards_->deleteLater();
    listViewCards_ = nullptr;
  }

  viewMode_ = mode;

  switch (mode) {
    case ListViewMode::List:
      listView_ = new ListView(this, model_, proxyModel_, AnimeListContext::Search);
      layout()->addWidget(listView_);
      listView_->show();
      break;

    case ListViewMode::Cards:
      listViewCards_ = new ListViewCards(this, model_, proxyModel_, AnimeListContext::Search);
      layout()->addWidget(listViewCards_);
      listViewCards_->show();
      break;
  }
}

sync::SearchParams SearchWidget::currentSearchParams() const {
  const auto& filters = proxyModel_->filters();

  return {
      .text = mainWindow()->searchBox()->text(),
      .year = filters.year,
      .season = filters.season ? std::make_optional(static_cast<anime::SeasonName>(*filters.season))
                               : std::nullopt,
      .type =
          filters.type ? std::make_optional(static_cast<anime::Type>(*filters.type)) : std::nullopt,
      .status = filters.status ? std::make_optional(static_cast<anime::Status>(*filters.status))
                               : std::nullopt,
      .sort = toSearchSort(static_cast<AnimeListModel::Column>(proxyModel_->sortColumn())),
      .sortOrder = proxyModel_->sortOrder(),
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
