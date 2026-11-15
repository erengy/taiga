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

#include "anime_list_view.hpp"

#include <QHeaderView>
#include <QKeyEvent>

#include "gui/common/anime_list_item_delegate.hpp"
#include "gui/common/anime_list_view_base.hpp"
#include "gui/common/header_view.hpp"
#include "gui/models/anime_list_model.hpp"
#include "gui/models/anime_list_proxy_model.hpp"
#include "gui/utils/painters.hpp"
#include "track/play.hpp"

namespace gui {

ListView::ListView(QWidget* parent, AnimeListModel* model, AnimeListProxyModel* proxyModel,
                   AnimeListContext context)
    : base_(new ListViewBase(parent, this, model, proxyModel, context)) {
  setObjectName("animeList");
  setProperty("themed", true);

  setFrameShape(QFrame::Shape::NoFrame);

  setAlternatingRowColors(true);
  setItemDelegate(new ListItemDelegate(this));

  setAllColumnsShowFocus(true);
  setExpandsOnDoubleClick(false);
  setItemsExpandable(false);
  setRootIsDecorated(false);
  setUniformRowHeights(true);

  HeaderView::install(this);

  header()->setFirstSectionMovable(true);
  header()->setStretchLastSection(false);
  header()->setTextElideMode(Qt::ElideRight);
  header()->hideSection(AnimeListModel::ColumnDuration);
  header()->hideSection(AnimeListModel::ColumnRewatches);
  header()->hideSection(AnimeListModel::ColumnStarted);
  header()->hideSection(AnimeListModel::ColumnCompleted);
  header()->hideSection(AnimeListModel::ColumnNotes);
  if (context == AnimeListContext::Search) {
    header()->hideSection(AnimeListModel::ColumnScore);
    header()->hideSection(AnimeListModel::ColumnLastUpdated);
  } else {
    header()->hideSection(AnimeListModel::ColumnAverage);
  }
  header()->resizeSection(AnimeListModel::ColumnTitle, 295);
  header()->resizeSection(AnimeListModel::ColumnProgress, 150);
  header()->resizeSection(AnimeListModel::ColumnDuration, 75);
  header()->resizeSection(AnimeListModel::ColumnScore, 75);
  header()->resizeSection(AnimeListModel::ColumnAverage, 85);
  header()->resizeSection(AnimeListModel::ColumnType, 75);
  header()->resizeSection(AnimeListModel::ColumnLastUpdated, 110);

  // `sortByColumn` needs to be called before `setSortingEnabled`.
  // Otherwise the sort column is set to `0`.
  sortByColumn(proxyModel->sortColumn(), proxyModel->sortOrder());
  setSortingEnabled(true);

  connect(this, &QAbstractItemView::clicked, this,
          qOverload<const QModelIndex&>(&QAbstractItemView::edit));
}

void ListView::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key::Key_Return || event->key() == Qt::Key::Key_Enter) {
    const auto indexes = selectionModel()->selectedRows();
    for (const auto& index : indexes) {
      base_->showMediaDialog(index);
    }
    return;
  }

  QTreeView::keyPressEvent(event);
}

void ListView::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::MouseButton::MiddleButton) {
    const QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
      setCurrentIndex(index);
      if (base_->context() == AnimeListContext::Search) {
        base_->openAnimePage(index);
      } else {
        base_->playNextEpisode(index);
      }
      return;
    }
  }

  QTreeView::mousePressEvent(event);
}

void ListView::paintEvent(QPaintEvent* event) {
  if (model() && model()->rowCount() == 0) {
    paintEmptyListText(this, tr("No items found."));
  }

  QTreeView::paintEvent(event);
}

}  // namespace gui
