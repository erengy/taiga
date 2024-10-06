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

#include "search_list_widget.hpp"

#include <QContextMenuEvent>

#include "gui/media/media_dialog.hpp"
#include "gui/media/media_menu.hpp"
#include "gui/models/anime_list_model.hpp"
#include "gui/models/anime_list_proxy_model.hpp"
#include "gui/search/search_list_item_delegate.hpp"
#include "media/anime.hpp"

namespace gui {

SearchListWidget::SearchListWidget(QWidget* parent)
    : QListView(parent),
      m_model(new AnimeListModel(this)),
      m_proxyModel(new AnimeListProxyModel(this)) {
  setItemDelegate(new SearchListItemDelegate(this));

  m_proxyModel->setSourceModel(m_model);
  setModel(m_proxyModel);

  setContextMenuPolicy(Qt::CustomContextMenu);
  setFrameShape(QFrame::Shape::NoFrame);
  setViewMode(QListView::ViewMode::IconMode);
  setResizeMode(QListView::ResizeMode::Adjust);
  setSpacing(16);
  setUniformItemSizes(true);
  setWordWrap(true);

  connect(this, &QWidget::customContextMenuRequested, this, [this]() {
    const auto selectedRows = selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) return;

    QList<Anime> items;
    for (auto selectedIndex : selectedRows) {
      if (const auto item = m_model->getAnime(m_proxyModel->mapToSource(selectedIndex))) {
        items.push_back(*item);
      }
    }

    auto* menu = new MediaMenu(this, items, {});
    menu->popup();
  });

  connect(this, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex& index) {
    const auto selectedItem = m_model->getAnime(m_proxyModel->mapToSource(index));
    if (!selectedItem) return;
    MediaDialog::show(this, *selectedItem, {});
  });
}

}  // namespace gui
