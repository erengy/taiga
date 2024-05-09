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

#include "search_list_model.hpp"

#include "media/anime_db.hpp"

namespace gui {

SearchListModel::SearchListModel(QObject* parent) : QAbstractListModel(parent) {
  anime::Database db;
  db.read();
  setAnime(db.data());
}

int SearchListModel::rowCount(const QModelIndex&) const {
  return m_data.size();
}

QVariant SearchListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return {};

  const auto item = m_data.value(index.row());

  switch (role) {
    case Qt::ItemDataRole::DisplayRole:
      return QString::fromStdString(item.titles.romaji);
  }

  return {};
}

bool SearchListModel::setData(const QModelIndex& index, const QVariant& value, int role) {
  return false;
}

std::optional<Anime> SearchListModel::getAnime(const QModelIndex& index) const {
  if (!index.isValid()) return std::nullopt;

  return m_data[index.row()];
}

void SearchListModel::setAnime(const QList<Anime>& items) {
  beginInsertRows({}, 0, items.size());
  m_data = items;
  endInsertRows();
}

}  // namespace gui
