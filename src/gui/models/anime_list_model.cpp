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

#include "anime_list_model.hpp"

#include <QApplication>
#include <QColor>
#include <QDateTime>
#include <QFont>
#include <QPalette>
#include <QSize>

#include "gui/utils/format.hpp"
#include "media/anime_db.hpp"
#include "media/anime_season.hpp"

namespace gui {

AnimeListModel::AnimeListModel(QObject* parent) : QAbstractListModel(parent) {
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

int AnimeListModel::rowCount(const QModelIndex&) const {
  return m_ids.size();
}

int AnimeListModel::columnCount(const QModelIndex&) const {
  return NUM_COLUMNS;
}

QVariant AnimeListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return {};

  const auto anime = getAnime(index);
  if (!anime) return {};

  const auto entry = getListEntry(index);

  switch (role) {
    case Qt::DisplayRole:
      switch (index.column()) {
        case COLUMN_TITLE:
          return QString::fromStdString(anime->titles.romaji);
        case COLUMN_DURATION:
          return formatEpisodeLength(anime->episode_length);
        case COLUMN_REWATCHES:
          if (entry) return entry->rewatched_times;
          break;
        case COLUMN_SCORE:
          if (entry) return formatListScore(entry->score);
          break;
        case COLUMN_AVERAGE:
          return formatScore(anime->score);
        case COLUMN_TYPE:
          return formatType(anime->type);
        case COLUMN_SEASON:
          return formatSeason(anime::Season(anime->date_started));
        case COLUMN_STARTED:
          if (entry) return formatFuzzyDate(entry->date_started);
          break;
        case COLUMN_COMPLETED:
          if (entry) return formatFuzzyDate(entry->date_completed);
          break;
        case COLUMN_LAST_UPDATED:
          if (entry) return formatAsRelativeTime(entry->last_updated);
          break;
        case COLUMN_NOTES:
          if (entry) return QString::fromStdString(entry->notes);
          break;
      }
      break;

    case Qt::ToolTipRole:
      switch (index.column()) {
        case COLUMN_TITLE:
          return QString::fromStdString(anime->titles.romaji);
        case COLUMN_SEASON:
          return formatFuzzyDate(anime->date_started);
        case COLUMN_LAST_UPDATED:
          if (entry) return formatTimestamp(entry->last_updated);
          break;
        case COLUMN_NOTES:
          if (entry) return QString::fromStdString(entry->notes);
          break;
      }
      break;

    case Qt::TextAlignmentRole: {
      switch (index.column()) {
        case COLUMN_PROGRESS:
        case COLUMN_REWATCHES:
        case COLUMN_SCORE:
        case COLUMN_AVERAGE:
        case COLUMN_TYPE:
          return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
        case COLUMN_DURATION:
        case COLUMN_SEASON:
        case COLUMN_STARTED:
        case COLUMN_COMPLETED:
        case COLUMN_LAST_UPDATED:
          return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        default:
          return {};
      }
      break;
    }

    case Qt::ForegroundRole: {
      const auto disabledTextColor =
          qApp->palette().color(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Text);
      switch (index.column()) {
        case COLUMN_AVERAGE:
          if (!anime->score) return disabledTextColor;
          break;
        case COLUMN_DURATION:
          if (anime->episode_length < 1) return disabledTextColor;
          break;
        case COLUMN_SEASON:
          if (!anime->date_started) return disabledTextColor;
          break;
        case COLUMN_TYPE:
          if (anime->type == anime::Type::Unknown) return disabledTextColor;
          break;
        case COLUMN_SCORE:
          if (entry && !entry->score) return disabledTextColor;
          break;
        case COLUMN_STARTED:
          if (entry && !entry->date_started) return disabledTextColor;
          break;
        case COLUMN_COMPLETED:
          if (entry && !entry->date_completed) return disabledTextColor;
          break;
        case COLUMN_LAST_UPDATED:
          if (entry && !entry->last_updated) return disabledTextColor;
          break;
      }
      break;
    }

    case static_cast<int>(AnimeListItemDataRole::Anime): {
      return QVariant::fromValue(anime);
    }
    case static_cast<int>(AnimeListItemDataRole::ListEntry): {
      return QVariant::fromValue(entry);
    }
  }

  return {};
}

bool AnimeListModel::setData(const QModelIndex& index, const QVariant& value, int role) {
  if (index.isValid() && role == Qt::EditRole) {
    if (index.column() == COLUMN_SCORE) {
      // @TODO: Add to queue instead of directly modifying the entry
      const int id = m_ids.at(index.row());
      const auto entry = m_entries.find(id);
      entry->score = value.toString().toInt();
      emit dataChanged(index, index, {role});
      return true;
    }
  }
  return false;
}

QVariant AnimeListModel::headerData(int section, Qt::Orientation orientation, int role) const {
  switch (role) {
    case Qt::DisplayRole: {
      switch (section) {
        case COLUMN_TITLE:
          return tr("Title");
        case COLUMN_PROGRESS:
          return tr("Progress");
        case COLUMN_DURATION:
          return tr("Duration");
        case COLUMN_REWATCHES:
          return tr("Rewatches");
        case COLUMN_SCORE:
          return tr("Score");
        case COLUMN_AVERAGE:
          return tr("Average");
        case COLUMN_TYPE:
          return tr("Type");
        case COLUMN_SEASON:
          return tr("Season");
        case COLUMN_STARTED:
          return tr("Started");
        case COLUMN_COMPLETED:
          return tr("Completed");
        case COLUMN_LAST_UPDATED:
          return tr("Last updated");
        case COLUMN_NOTES:
          return tr("Notes");
      }
      break;
    }

    case Qt::TextAlignmentRole: {
      switch (section) {
        case COLUMN_PROGRESS:
        case COLUMN_REWATCHES:
        case COLUMN_SCORE:
        case COLUMN_AVERAGE:
        case COLUMN_TYPE:
          return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
        case COLUMN_DURATION:
        case COLUMN_SEASON:
        case COLUMN_STARTED:
        case COLUMN_COMPLETED:
        case COLUMN_LAST_UPDATED:
          return QVariant(Qt::AlignRight | Qt::AlignVCenter);
      }
      break;
    }

    case Qt::InitialSortOrderRole: {
      switch (section) {
        case COLUMN_PROGRESS:
        case COLUMN_DURATION:
        case COLUMN_REWATCHES:
        case COLUMN_SCORE:
        case COLUMN_AVERAGE:
        case COLUMN_SEASON:
        case COLUMN_STARTED:
        case COLUMN_COMPLETED:
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

Qt::ItemFlags AnimeListModel::flags(const QModelIndex& index) const {
  if (!index.isValid()) return Qt::NoItemFlags;

  return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
}

const Anime* AnimeListModel::getAnime(const QModelIndex& index) const {
  if (!index.isValid()) return nullptr;
  const int id = m_ids.at(index.row());
  const auto it = m_anime.find(id);
  return it != m_anime.end() ? &*it : nullptr;
}

const ListEntry* AnimeListModel::getListEntry(const QModelIndex& index) const {
  if (!index.isValid()) return nullptr;
  const int id = m_ids.at(index.row());
  const auto it = m_entries.find(id);
  return it != m_entries.end() ? &*it : nullptr;
}

}  // namespace gui
