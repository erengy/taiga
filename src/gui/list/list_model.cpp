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

#include "list_model.hpp"

#include <QColor>
#include <QFont>
#include <QSize>

#include "gui/utils/format.hpp"
#include "media/anime_db.hpp"
#include "media/anime_season.hpp"

namespace gui {

ListModel::ListModel(QObject* parent) : QAbstractListModel(parent) {
  anime::Database db;
  db.read();
  setAnime(db.data());
}

int ListModel::rowCount(const QModelIndex&) const {
  return m_data.size();
}

int ListModel::columnCount(const QModelIndex&) const {
  return NUM_COLUMNS;
}

QVariant ListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return {};

  const auto& item = m_data.at(index.row());

  switch (role) {
    case Qt::DisplayRole:
      switch (index.column()) {
        case COLUMN_TITLE:
          return QString::fromStdString(item.titles.romaji);
        case COLUMN_SCORE:
          return "-";
        case COLUMN_TYPE:
          return fromType(item.type);
        case COLUMN_SEASON:
          return fromSeason(anime::Season(item.start_date));
      }
      break;

    case Qt::ToolTipRole:
      switch (index.column()) {
        case COLUMN_TITLE:
          return QString::fromStdString(item.titles.romaji);
      }
      break;

    case Qt::TextAlignmentRole: {
      switch (index.column()) {
        case COLUMN_PROGRESS:
        case COLUMN_SCORE:
        case COLUMN_TYPE:
          return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
        case COLUMN_SEASON:
          return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        default:
          return {};
      }
      break;
    }

    case Qt::UserRole: {
      return QVariant::fromValue(&item);
    }
  }

  return {};
}

bool ListModel::setData(const QModelIndex& index, const QVariant& value, int role) {
  return false;
}

QVariant ListModel::headerData(int section, Qt::Orientation orientation, int role) const {
  switch (role) {
    case Qt::DisplayRole: {
      switch (section) {
        case COLUMN_TITLE:
          return tr("Title");
        case COLUMN_PROGRESS:
          return tr("Progress");
        case COLUMN_SCORE:
          return tr("Score");
        case COLUMN_TYPE:
          return tr("Type");
        case COLUMN_SEASON:
          return tr("Season");
      }
      break;
    }

    case Qt::TextAlignmentRole: {
      switch (section) {
        case COLUMN_PROGRESS:
        case COLUMN_SCORE:
        case COLUMN_TYPE:
          return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
        case COLUMN_SEASON:
          return QVariant(Qt::AlignRight | Qt::AlignVCenter);
      }
      break;
    }
  }

  return QAbstractListModel::headerData(section, orientation, role);
}

Qt::ItemFlags ListModel::flags(const QModelIndex& index) const {
  if (!index.isValid()) return Qt::NoItemFlags;

  return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
}

std::optional<Anime> ListModel::getAnime(const QModelIndex& index) const {
  if (!index.isValid()) return std::nullopt;

  return m_data.at(index.row());
}

void ListModel::setAnime(const QList<Anime>& items) {
  beginInsertRows({}, 0, items.size());
  m_data = items;
  endInsertRows();
}

}  // namespace gui
