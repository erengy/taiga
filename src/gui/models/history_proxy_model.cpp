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

#include "history_proxy_model.hpp"

#include "base/string.hpp"
#include "gui/models/history_model.hpp"
#include "media/anime_db.hpp"
#include "media/anime_history.hpp"

namespace {

const anime::HistoryItem* getHistoryItem(const QModelIndex& index) {
  const int role = static_cast<int>(gui::HistoryItemDataRole::HistoryItem);
  return index.data(role).value<const anime::HistoryItem*>();
}

}  // namespace

namespace gui {

HistoryProxyModel::HistoryProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

bool HistoryProxyModel::lessThan(const QModelIndex& lhs, const QModelIndex& rhs) const {
  const auto lhsItem = getHistoryItem(lhs);
  const auto rhsItem = getHistoryItem(rhs);
  if (!lhsItem || !rhsItem) return false;

  switch (lhs.column()) {
    case HistoryModel::COLUMN_TITLE: {
      const auto lhsAnime = anime::db.item(lhsItem->anime_id);
      const auto rhsAnime = anime::db.item(rhsItem->anime_id);
      const auto lhsTitle = lhsAnime ? lhsAnime->titles.romaji : std::string{};
      const auto rhsTitle = rhsAnime ? rhsAnime->titles.romaji : std::string{};
      return compareStrings(lhsTitle, rhsTitle, Qt::CaseInsensitive) < 0;
    }

    case HistoryModel::COLUMN_DETAILS:
      return lhsItem->episode < rhsItem->episode;

    case HistoryModel::COLUMN_MODIFIED:
    default:
      return lhsItem->time < rhsItem->time;
  }
}

}  // namespace gui
