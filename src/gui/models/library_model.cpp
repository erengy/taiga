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

#include "library_model.hpp"

namespace gui {

LibraryModel::LibraryModel(QObject* parent) : QFileSystemModel(parent) {}

QVariant LibraryModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return {};

  switch (role) {
    case Qt::TextAlignmentRole: {
      switch (index.column()) {
        case COLUMN_SIZE:
        case COLUMN_MODIFIED:
          return QVariant(Qt::AlignRight | Qt::AlignVCenter);
      }
      break;
    }
  }

  return QFileSystemModel::data(index, role);
}

QVariant LibraryModel::headerData(int section, Qt::Orientation orientation, int role) const {
  switch (role) {
    case Qt::DisplayRole: {
      switch (section) {
        case COLUMN_TITLE:
          return tr("Title");
        case COLUMN_SIZE:
          return tr("Size");
        case COLUMN_TYPE:
          return tr("Type");
        case COLUMN_MODIFIED:
          return tr("Last modified");
      }
      break;
    }

    case Qt::TextAlignmentRole: {
      switch (section) {
        case COLUMN_TITLE:
        case COLUMN_TYPE:
          return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        case COLUMN_SIZE:
        case COLUMN_MODIFIED:
          return QVariant(Qt::AlignRight | Qt::AlignVCenter);
      }
      break;
    }

    case Qt::InitialSortOrderRole: {
      switch (section) {
        case COLUMN_SIZE:
        case COLUMN_MODIFIED:
          return Qt::DescendingOrder;
        default:
          return Qt::AscendingOrder;
      }
      break;
    }
  }

  return QFileSystemModel::headerData(section, orientation, role);
}

}  // namespace gui
