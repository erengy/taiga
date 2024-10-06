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

#include "list_proxy_model.hpp"

#include "gui/list/list_item_delegate.hpp"
#include "gui/list/list_model.hpp"
#include "media/anime.hpp"

namespace {

const Anime* getAnime(const QModelIndex& index) {
  return index.data(static_cast<int>(gui::ListItemDataRole::Anime)).value<const Anime*>();
}

const ListEntry* getListEntry(const QModelIndex& index) {
  return index.data(static_cast<int>(gui::ListItemDataRole::ListEntry)).value<const ListEntry*>();
}

}  // namespace

namespace gui {

ListProxyModel::ListProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {
  setFilterKeyColumn(ListModel::COLUMN_TITLE);
  setFilterRole(Qt::DisplayRole);

  setSortCaseSensitivity(Qt::CaseInsensitive);
  setSortRole(Qt::UserRole);

  setListStatusFilter(static_cast<int>(anime::list::Status::Watching));
}

void ListProxyModel::removeListStatusFilter() {
  m_filter.listStatus.reset();
  invalidateRowsFilter();
}

void ListProxyModel::setListStatusFilter(int status) {
  m_filter.listStatus = status;
  invalidateRowsFilter();
}

void ListProxyModel::setTextFilter(const QString& text) {
  m_filter.text = text;
  invalidateRowsFilter();
}

bool ListProxyModel::filterAcceptsRow(int row, const QModelIndex& parent) const {
  const auto model = static_cast<ListModel*>(sourceModel());
  if (!model) return false;

  const auto index = model->index(row, 0, parent);
  const auto anime = getAnime(index);
  const auto entry = getListEntry(index);
  if (!anime) return false;

  if (m_filter.listStatus.has_value()) {
    const auto status = static_cast<int>(entry ? entry->status : anime::list::Status::NotInList);
    if (status != m_filter.listStatus.value()) return false;
  }
  if (!m_filter.text.isEmpty()) {
    if (!anime->titles.romaji.contains(m_filter.text.toStdString())) return false;
  }

  return QSortFilterProxyModel::filterAcceptsRow(row, parent);
}

bool ListProxyModel::lessThan(const QModelIndex& lhs, const QModelIndex& rhs) const {
  const auto lhs_anime = getAnime(lhs);
  const auto lhs_entry = getListEntry(lhs);
  const auto rhs_anime = getAnime(rhs);
  const auto rhs_entry = getListEntry(rhs);

  if (!lhs_anime || !rhs_anime) return false;

  switch (lhs.column()) {
    case ListModel::COLUMN_TITLE:
      return lhs_anime->titles.romaji < rhs_anime->titles.romaji;
    case ListModel::COLUMN_TYPE:
      return lhs_anime->type < rhs_anime->type;
    case ListModel::COLUMN_PROGRESS:
      return (lhs_entry ? lhs_entry->watched_episodes : 0) <
             (rhs_entry ? rhs_entry->watched_episodes : 0);
    case ListModel::COLUMN_SCORE:
      return (lhs_entry ? lhs_entry->score : 0) < (rhs_entry ? rhs_entry->score : 0);
    case ListModel::COLUMN_SEASON:
      return lhs_anime->start_date < rhs_anime->start_date;
    case ListModel::COLUMN_LAST_UPDATED:
      return (lhs_entry ? lhs_entry->last_updated : "") <
             (rhs_entry ? rhs_entry->last_updated : "");
  }

  return false;
}

}  // namespace gui
