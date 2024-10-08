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

#include <QAbstractListModel>
#include <QList>
#include <QMap>

#include "media/anime.hpp"

namespace gui {

enum class AnimeListItemDataRole {
  Anime = Qt::UserRole,
  ListEntry,
};

class AnimeListModel final : public QAbstractListModel {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(AnimeListModel)

public:
  enum Column {
    COLUMN_TITLE,
    COLUMN_PROGRESS,
    COLUMN_SCORE,
    COLUMN_AVERAGE,
    COLUMN_TYPE,
    COLUMN_SEASON,
    COLUMN_LAST_UPDATED,
    NUM_COLUMNS
  };

  AnimeListModel(QObject* parent);
  ~AnimeListModel() = default;

  int rowCount(const QModelIndex& parent = {}) const override;
  int columnCount(const QModelIndex& parent = {}) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role) override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  const Anime* getAnime(const QModelIndex& index) const;
  const ListEntry* getListEntry(const QModelIndex& index) const;

private:
  QList<int> m_ids;
  QMap<int, Anime> m_anime;
  QMap<int, ListEntry> m_entries;
};

}  // namespace gui
