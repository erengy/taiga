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

#include "search_list_proxy_model.hpp"

#include "gui/search/search_list_item_delegate.hpp"
#include "gui/search/search_list_model.hpp"
#include "media/anime.hpp"
#include "media/anime_season.hpp"

namespace gui {

SearchListProxyModel::SearchListProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {
  setFilterKeyColumn(0);
  setFilterRole(Qt::DisplayRole);
  setSortCaseSensitivity(Qt::CaseInsensitive);
  setSortRole(Qt::UserRole);
}

void SearchListProxyModel::setYearFilter(int year) {
  m_filter.year = year;
  invalidateRowsFilter();
}

void SearchListProxyModel::setSeasonFilter(int season) {
  m_filter.season = season;
  invalidateRowsFilter();
}

void SearchListProxyModel::setTypeFilter(int type) {
  m_filter.type = type;
  invalidateRowsFilter();
}

void SearchListProxyModel::setStatusFilter(int status) {
  m_filter.status = status;
  invalidateRowsFilter();
}

bool SearchListProxyModel::filterAcceptsRow(int row, const QModelIndex& parent) const {
  const auto model = static_cast<SearchListModel*>(sourceModel());
  if (!model) return false;

  const auto anime = model->index(row, 0, parent)
                         .data(static_cast<int>(SearchListItemDataRole::Anime))
                         .value<const Anime*>();
  if (!anime) return false;

  if (m_filter.year) {
    if (anime->start_date.year() != *m_filter.year) return false;
  }
  if (m_filter.season) {
    const anime::Season season{anime->start_date};
    if (static_cast<int>(season.name) != *m_filter.season) return false;
  }
  if (m_filter.type) {
    if (static_cast<int>(anime->type) != *m_filter.type) return false;
  }
  if (m_filter.status) {
    if (static_cast<int>(anime->status) != *m_filter.status) return false;
  }

  return QSortFilterProxyModel::filterAcceptsRow(row, parent);
}

}  // namespace gui
