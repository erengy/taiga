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

#include "history_model.hpp"

#include "gui/utils/format.hpp"
#include "media/anime_db.hpp"
#include "media/anime_history.hpp"
#include "media/anime_utils.hpp"
#include "taiga/settings.hpp"

namespace gui {

HistoryModel::HistoryModel(QObject* parent) : QAbstractListModel(parent) {
  connect(anime::history(), &anime::History::changed, this, [this]() {
    beginResetModel();
    endResetModel();
  });

  connect(taiga::settings(), &taiga::Settings::changed, this, [this](const QSet<QString>& keys) {
    if (!keys.contains(taiga::Settings::Key::kAppTitleLanguage)) return;
    if (const auto rows = rowCount(); rows > 0) {
      emit dataChanged(index(0, ColumnTitle), index(rows - 1, ColumnTitle));
    }
  });
}

int HistoryModel::rowCount(const QModelIndex&) const {
  return anime::history()->items().size();
}

int HistoryModel::columnCount(const QModelIndex&) const {
  return NumColumns;
}

QVariant HistoryModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return {};

  const auto& historyItem = anime::history()->items().at(index.row());

  switch (role) {
    case Qt::DisplayRole: {
      const auto item = anime::db()->item(historyItem.animeId);
      switch (index.column()) {
        case ColumnTitle:
          if (item) return QString::fromStdString(anime::preferredTitle(*item));
          return {};
        case ColumnDetails:
          return tr("Episode: %1").arg(historyItem.episode);
        case ColumnModified:
          return formatAsRelativeTime(historyItem.time, "-");
      }
      break;
    }

    case Qt::ToolTipRole: {
      switch (index.column()) {
        case ColumnModified:
          return formatTimestamp(historyItem.time);
      }
      break;
    }

    case Qt::TextAlignmentRole: {
      switch (index.column()) {
        case ColumnModified:
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
        case ColumnTitle: return tr("Title");
        case ColumnDetails: return tr("Details");
        case ColumnModified: return tr("Last modified");
      }
      // clang-format on
      break;
    }

    case Qt::TextAlignmentRole: {
      switch (section) {
        case ColumnTitle:
        case ColumnDetails:
          return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        case ColumnModified:
          return QVariant(Qt::AlignRight | Qt::AlignVCenter);
      }
      break;
    }

    case Qt::InitialSortOrderRole: {
      switch (section) {
        case ColumnModified:
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
