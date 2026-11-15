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

#pragma once

#include <QMenu>

#include "gui/common/anime_list_view_base.hpp"
#include "gui/common/page_widget.hpp"

namespace gui {

class AnimeListModel;
class AnimeListProxyModel;
class ListView;
class ListViewCards;

class ListWidget final : public PageWidget {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(ListWidget)

public:
  ListWidget(QWidget* parent);

  ListViewMode viewMode() const;
  void setViewMode(ListViewMode mode);

  void saveState();

private:
  void initToolbar();
  void initSortMenu();
  void initViewMenu();
  void initMoreMenu();

  AnimeListModel* model_ = nullptr;
  AnimeListProxyModel* proxyModel_ = nullptr;
  ListView* listView_ = nullptr;
  ListViewCards* listViewCards_ = nullptr;
  ListViewMode viewMode_ = ListViewMode::List;
  QMenu* sortMenu_ = nullptr;
  QMenu* viewMenu_ = nullptr;
  QMenu* moreMenu_ = nullptr;
};

}  // namespace gui
