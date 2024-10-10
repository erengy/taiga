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

#include "anime_list_view.hpp"

#include <QHeaderView>

#include "gui/common/anime_list_item_delegate.hpp"
#include "gui/common/anime_list_view_base.hpp"
#include "gui/main/main_window.hpp"
#include "gui/models/anime_list_model.hpp"
#include "gui/models/anime_list_proxy_model.hpp"

namespace gui {

ListView::ListView(QWidget* parent, AnimeListModel* model, AnimeListProxyModel* proxyModel,
                   MainWindow* mainWindow)
    : m_base(new ListViewBase(parent, this, model, proxyModel, mainWindow)) {
  setObjectName("animeList");

  setFrameShape(QFrame::Shape::NoFrame);

  setAlternatingRowColors(true);
  setItemDelegate(new ListItemDelegate(this));

  setAllColumnsShowFocus(true);
  setExpandsOnDoubleClick(false);
  setItemsExpandable(false);
  setRootIsDecorated(false);
  setSortingEnabled(true);
  setUniformRowHeights(true);

  header()->setFirstSectionMovable(true);
  header()->setStretchLastSection(false);
  header()->setTextElideMode(Qt::ElideRight);
  header()->hideSection(AnimeListModel::COLUMN_DURATION);
  header()->hideSection(AnimeListModel::COLUMN_REWATCHES);
  header()->hideSection(AnimeListModel::COLUMN_AVERAGE);
  header()->hideSection(AnimeListModel::COLUMN_STARTED);
  header()->hideSection(AnimeListModel::COLUMN_COMPLETED);
  header()->hideSection(AnimeListModel::COLUMN_NOTES);
  header()->resizeSection(AnimeListModel::COLUMN_TITLE, 295);
  header()->resizeSection(AnimeListModel::COLUMN_PROGRESS, 150);
  header()->resizeSection(AnimeListModel::COLUMN_DURATION, 75);
  header()->resizeSection(AnimeListModel::COLUMN_SCORE, 75);
  header()->resizeSection(AnimeListModel::COLUMN_AVERAGE, 75);
  header()->resizeSection(AnimeListModel::COLUMN_TYPE, 75);
  header()->resizeSection(AnimeListModel::COLUMN_LAST_UPDATED, 110);

  sortByColumn(AnimeListModel::COLUMN_LAST_UPDATED, Qt::SortOrder::DescendingOrder);

  connect(this, &QAbstractItemView::clicked, this,
          qOverload<const QModelIndex&>(&QAbstractItemView::edit));
}

}  // namespace gui
