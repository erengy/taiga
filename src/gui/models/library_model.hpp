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

#pragma once

#include <QFileSystemModel>

namespace gui {

class LibraryModel final : public QFileSystemModel {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(LibraryModel)

public:
  enum Column {
    COLUMN_NAME,
    COLUMN_SIZE,
    COLUMN_TYPE,
    COLUMN_MODIFIED,
    COLUMN_ANIME,
    COLUMN_EPISODE,
    NUM_COLUMNS,
  };

  LibraryModel(QObject* parent);
  ~LibraryModel() = default;

  int columnCount(const QModelIndex& parent = {}) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
};

}  // namespace gui
