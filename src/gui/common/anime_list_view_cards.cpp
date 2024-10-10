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

#include "anime_list_view_cards.hpp"

#include <QKeyEvent>

#include "gui/common/anime_list_item_delegate_cards.hpp"
#include "gui/common/anime_list_view_base.hpp"
#include "gui/main/main_window.hpp"
#include "gui/models/anime_list_model.hpp"
#include "gui/models/anime_list_proxy_model.hpp"

namespace gui {

ListViewCards::ListViewCards(QWidget* parent, AnimeListModel* model,
                             AnimeListProxyModel* proxyModel, MainWindow* mainWindow)
    : m_base(new ListViewBase(parent, this, model, proxyModel, mainWindow)) {
  setFrameShape(QFrame::Shape::NoFrame);

  setItemDelegate(new ListItemDelegateCards(this));

  setViewMode(QListView::ViewMode::IconMode);
  setResizeMode(QListView::ResizeMode::Adjust);
  setSpacing(16);
  setUniformItemSizes(true);
  setWordWrap(true);
}

void ListViewCards::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key::Key_Return || event->key() == Qt::Key::Key_Enter) {
    const auto indexes = selectionModel()->selectedIndexes();
    for (const auto& index : indexes) {
      m_base->showMediaDialog(index);
    }
    return;
  }

  QListView::keyPressEvent(event);
}

}  // namespace gui
