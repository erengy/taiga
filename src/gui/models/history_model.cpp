/**
 * Taiga
 * Copyright (C) 2010-2025, Eren Okka
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

#include "history_model.hpp"

#include "gui/utils/format.hpp"
#include "media/anime_db.hpp"
#include "media/anime_history.hpp"

namespace gui {

HistoryModel::HistoryModel(QObject* parent) : QAbstractListModel(parent) {}

int HistoryModel::rowCount(const QModelIndex&) const {
  return anime::history.items().size();
}

int HistoryModel::columnCount(const QModelIndex&) const {
  return NUM_COLUMNS;
}

QVariant HistoryModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return {};

  const auto& historyItem = anime::history.items().at(index.row());

  switch (role) {
    case Qt::DisplayRole: {
      const auto item = anime::db.item(historyItem.anime_id);
      switch (index.column()) {
        case COLUMN_TITLE:
          if (item) return QString::fromStdString(item->titles.romaji);
          return {};
        case COLUMN_DETAILS:
          return tr("Episode: %1").arg(historyItem.episode);
        case COLUMN_MODIFIED:
          return formatAsRelativeTime(historyItem.time, "-");
      }
      break;
    }

    case Qt::ToolTipRole: {
      switch (index.column()) {
        case COLUMN_MODIFIED:
          return formatTimestamp(historyItem.time);
      }
      break;
    }

    case Qt::TextAlignmentRole: {
      switch (index.column()) {
        case COLUMN_MODIFIED:
          return QVariant(Qt::AlignRight | Qt::AlignVCenter);
      }
      break;
    }

    case static_cast<int>(HistoryItemDataRole::HistoryItem):
      return QVariant::fromValue(&historyItem);
  }

  return {};
}

QVariant HistoryModel::headerData(int section, Qt::Orientation orientation, int role) const {
  switch (role) {
    case Qt::DisplayRole: {
      // clang-format off
      switch (section) {
        case COLUMN_TITLE: return tr("Title");
        case COLUMN_DETAILS: return tr("Details");
        case COLUMN_MODIFIED: return tr("Last modified");
      }
      // clang-format on
      break;
    }

    case Qt::TextAlignmentRole: {
      switch (section) {
        case COLUMN_TITLE:
        case COLUMN_DETAILS:
          return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        case COLUMN_MODIFIED:
          return QVariant(Qt::AlignRight | Qt::AlignVCenter);
      }
      break;
    }

    case Qt::InitialSortOrderRole: {
      switch (section) {
        case COLUMN_MODIFIED:
          return Qt::DescendingOrder;
        default:
          return Qt::AscendingOrder;
      }
      break;
    }
  }

  return QAbstractListModel::headerData(section, orientation, role);
}

}  // namespace gui
