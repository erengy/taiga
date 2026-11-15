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

#include "list_widget.hpp"

#include <QActionGroup>
#include <QDateTime>
#include <QFileDialog>
#include <QListView>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>
#include <format>

#include "base/string.hpp"
#include "gui/common/anime_list_context.hpp"
#include "gui/common/anime_list_view.hpp"
#include "gui/common/anime_list_view_cards.hpp"
#include "gui/main/main_window.hpp"
#include "gui/main/navigation_widget.hpp"
#include "gui/main/status_bar_controller.hpp"
#include "gui/models/anime_list_model.hpp"
#include "gui/models/anime_list_proxy_model.hpp"
#include "gui/utils/theme.hpp"
#include "media/anime_list_export.hpp"
#include "taiga/session.hpp"

namespace gui {

ListWidget::ListWidget(QWidget* parent)
    : PageWidget(parent),
      model_(new AnimeListModel(this)),
      proxyModel_(new AnimeListProxyModel(this)),
      sortMenu_(new QMenu(this)),
      viewMenu_(new QMenu(this)),
      moreMenu_(new QMenu(this)) {
  proxyModel_->setSecondarySort(taiga::session.animeListSecondarySortColumn(),
                                taiga::session.animeListSecondarySortOrder());
  proxyModel_->sort(taiga::session.animeListSortColumn(), taiga::session.animeListSortOrder());

  initToolbar();
  setViewMode(taiga::session.animeListViewMode());

  connect(sortMenu_, &QMenu::aboutToShow, this, &ListWidget::initSortMenu);
  connect(viewMenu_, &QMenu::aboutToShow, this, &ListWidget::initViewMenu);
  connect(moreMenu_, &QMenu::aboutToShow, this, &ListWidget::initMoreMenu);

  connect(mainWindow()->navigation(), &NavigationWidget::currentListStatusChanged, this,
          [this](anime::list::Status status) {
            proxyModel_->setListStatusFilter({
                .status = static_cast<int>(status),
                .anyStatus = !static_cast<int>(status),
            });
          });
}

ListViewMode ListWidget::viewMode() const {
  return viewMode_;
}

void ListWidget::setViewMode(ListViewMode mode) {
  if (listView_) {
    layout()->removeWidget(listView_);
    listView_->deleteLater();
    listView_ = nullptr;
  };
  if (listViewCards_) {
    layout()->removeWidget(listViewCards_);
    listViewCards_->deleteLater();
    listViewCards_ = nullptr;
  };

  viewMode_ = mode;

  switch (mode) {
    case ListViewMode::List:
      listView_ = new ListView(this, model_, proxyModel_, AnimeListContext::List);
      layout()->addWidget(listView_);
      listView_->show();
      break;

    case ListViewMode::Cards:
      listViewCards_ = new ListViewCards(this, model_, proxyModel_, AnimeListContext::List);
      layout()->addWidget(listViewCards_);
      listViewCards_->show();
      break;
  }
}

void ListWidget::saveState() {
  taiga::session.setAnimeListSortColumn(proxyModel_->sortColumn());
  taiga::session.setAnimeListSortOrder(proxyModel_->sortOrder());
  taiga::session.setAnimeListSecondarySortColumn(proxyModel_->secondarySortColumn());
  taiga::session.setAnimeListSecondarySortOrder(proxyModel_->secondarySortOrder());
  taiga::session.setAnimeListViewMode(viewMode_);
}

void ListWidget::initToolbar() {
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

void ListWidget::initSortMenu() {
  using Qt::SortOrder::AscendingOrder;
  using Qt::SortOrder::DescendingOrder;

  static const QList<QPair<AnimeListModel::Column, Qt::SortOrder>> items{
      {AnimeListModel::ColumnTitle, AscendingOrder},
      {AnimeListModel::ColumnProgress, DescendingOrder},
      {AnimeListModel::ColumnDuration, DescendingOrder},
      {AnimeListModel::ColumnRewatches, DescendingOrder},
      {AnimeListModel::ColumnScore, DescendingOrder},
      {AnimeListModel::ColumnAverage, DescendingOrder},
      {AnimeListModel::ColumnType, AscendingOrder},
      {AnimeListModel::ColumnSeason, DescendingOrder},
      {AnimeListModel::ColumnStarted, DescendingOrder},
      {AnimeListModel::ColumnCompleted, DescendingOrder},
      {AnimeListModel::ColumnLastUpdated, DescendingOrder},
      {AnimeListModel::ColumnNotes, AscendingOrder},
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
    });

    action->setCheckable(true);
    action->setChecked(column == proxyModel_->sortColumn());
    actionGroup->addAction(action);
  }
}

void ListWidget::initViewMenu() {
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

void ListWidget::initMoreMenu() {
  moreMenu_->clear();

  static constexpr auto exportAs = [](QWidget* parent, const QString& extension,
                                      auto exportFunction) {
    const auto directory = QFileDialog::getExistingDirectory(
        parent, tr("Select Export Location"), {},
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks | QFileDialog::ReadOnly);

    if (directory.isEmpty()) return;

    const auto timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();
    const auto path = u"%1/animelist_%2.%3"_s.arg(directory).arg(timestamp).arg(extension);
    const auto success = exportFunction(path.toStdString());

    mainWindow()->statusBarController()->showMessage({
        .source = StatusBarController::Source::Export,
        .text = success ? tr("Exported list to %1.").arg(path)
                        : tr("Could not export list to %1.").arg(path),
        .spin = false,
    });
  };

  moreMenu_->addAction(tr("Export as Markdown..."), this,
                       [this]() { exportAs(this, "md", &anime::list::exportAsMarkdown); });

  moreMenu_->addAction(tr("Export as XML..."), this,
                       [this]() { exportAs(this, "xml", &anime::list::exportAsXml); });
}

}  // namespace gui
