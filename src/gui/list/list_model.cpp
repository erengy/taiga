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
#include <QDateTime>
#include <QFont>
#include <QSize>

#include "gui/list/list_item_delegate.hpp"
#include "gui/utils/format.hpp"
#include "media/anime_db.hpp"
#include "media/anime_season.hpp"

namespace gui {

ListModel::ListModel(QObject* parent) : QAbstractListModel(parent) {
  const auto db = anime::readDatabase();
  const auto entries = anime::readListEntries();

  beginInsertRows({}, 0, db.size());

  for (const auto& anime : db) {
    m_anime[anime.id] = anime;
    m_ids.push_back(anime.id);
  }
  for (const auto& entry : entries) {
    m_entries[entry.anime_id] = entry;
  }

  endInsertRows();
}

int ListModel::rowCount(const QModelIndex&) const {
  return m_ids.size();
}

int ListModel::columnCount(const QModelIndex&) const {
  return NUM_COLUMNS;
}

QVariant ListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return {};

  const auto anime = getAnime(index);
  if (!anime) return {};

  const auto entry = getListEntry(index);

  switch (role) {
    case Qt::DisplayRole:
      switch (index.column()) {
        case COLUMN_TITLE:
          return QString::fromStdString(anime->titles.romaji);
        case COLUMN_SCORE:
          if (entry) return formatListScore(entry->score);
          break;
        case COLUMN_TYPE:
          return formatType(anime->type);
        case COLUMN_SEASON:
          return formatSeason(anime::Season(anime->start_date));
        case COLUMN_LAST_UPDATED:
          if (entry) {
            const auto time = QString::fromStdString(entry->last_updated).toLongLong();
            return formatAsRelativeTime(time);
          }
          break;
      }
      break;

    case Qt::ToolTipRole:
      switch (index.column()) {
        case COLUMN_TITLE:
          return QString::fromStdString(anime->titles.romaji);
        case COLUMN_SEASON:
          return formatFuzzyDate(anime->start_date);
        case COLUMN_LAST_UPDATED:
          if (entry) return QString::fromStdString(entry->last_updated);  // @TODO: Format as date
          break;
      }
      break;

    case Qt::TextAlignmentRole: {
      switch (index.column()) {
        case COLUMN_PROGRESS:
        case COLUMN_SCORE:
        case COLUMN_TYPE:
          return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
        case COLUMN_SEASON:
        case COLUMN_LAST_UPDATED:
          return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        default:
          return {};
      }
      break;
    }

    case static_cast<int>(ListItemDataRole::Anime): {
      return QVariant::fromValue(anime);
    }
    case static_cast<int>(ListItemDataRole::ListEntry): {
      return QVariant::fromValue(entry);
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
        case COLUMN_LAST_UPDATED:
          return tr("Last updated");
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
        case COLUMN_LAST_UPDATED:
          return QVariant(Qt::AlignRight | Qt::AlignVCenter);
      }
      break;
    }

    case Qt::InitialSortOrderRole: {
      switch (section) {
        case COLUMN_PROGRESS:
        case COLUMN_SCORE:
        case COLUMN_SEASON:
        case COLUMN_LAST_UPDATED:
          return Qt::DescendingOrder;
        default:
          return Qt::AscendingOrder;
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

const Anime* ListModel::getAnime(const QModelIndex& index) const {
  if (!index.isValid()) return nullptr;
  const int id = m_ids.at(index.row());
  const auto it = m_anime.find(id);
  return it != m_anime.end() ? &*it : nullptr;
}

const ListEntry* ListModel::getListEntry(const QModelIndex& index) const {
  if (!index.isValid()) return nullptr;
  const int id = m_ids.at(index.row());
  const auto it = m_entries.find(id);
  return it != m_entries.end() ? &*it : nullptr;
}

}  // namespace gui
