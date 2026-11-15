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
#include "gui/common/combobox.hpp"
#include "gui/common/page_widget.hpp"

namespace sync {
struct SearchParams;
}

namespace gui {

class AnimeListModel;
class AnimeListProxyModel;
class ListView;
class ListViewCards;

class SearchWidget final : public PageWidget {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(SearchWidget)

public:
  SearchWidget(QWidget* parent);

  void saveState();

private:
  void initSortMenu();
  void initViewMenu();
  void initMoreMenu();
  void setViewMode(ListViewMode mode);

  sync::SearchParams currentSearchParams() const;
  void performSearch();

  AnimeListModel* model_ = nullptr;
  AnimeListProxyModel* proxyModel_ = nullptr;
  ComboBox* comboYear_ = nullptr;
  ComboBox* comboSeason_ = nullptr;
  ComboBox* comboType_ = nullptr;
  ComboBox* comboStatus_ = nullptr;
  ComboBox* comboListStatus_ = nullptr;
  ListView* listView_ = nullptr;
  ListViewCards* listViewCards_ = nullptr;
  ListViewMode viewMode_ = ListViewMode::Cards;
  QMenu* sortMenu_ = nullptr;
  QMenu* viewMenu_ = nullptr;
  QMenu* moreMenu_ = nullptr;
};

}  // namespace gui
