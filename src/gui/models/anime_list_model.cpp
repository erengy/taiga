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

#include "anime_list_model.hpp"

#include <QApplication>
#include <QColor>
#include <QDateTime>
#include <QFont>
#include <QPalette>
#include <QSize>

#include "gui/utils/format.hpp"
#include "gui/utils/image_provider.hpp"
#include "gui/utils/rating.hpp"
#include "media/anime_db.hpp"
#include "media/anime_list_utils.hpp"
#include "media/anime_season.hpp"
#include "media/anime_utils.hpp"
#include "taiga/settings.hpp"

namespace gui {

AnimeListModel::AnimeListModel(QObject* parent) : QAbstractListModel(parent) {
  beginInsertRows({}, 0, anime::db()->items().size());
  ids_ = anime::db()->items().keys();
  endInsertRows();

  connect(imageProvider(), &ImageProvider::posterChanged, this, [this](int id) {
    if (const auto row = ids_.indexOf(id); row > -1) {
      emit dataChanged(index(row), index(row), {static_cast<int>(AnimeListItemDataRole::Poster)});
    }
  });

  connect(anime::db(), &anime::Database::itemUpdated, this, &AnimeListModel::refreshRow);
  connect(anime::db(), &anime::Database::itemDeleted, this, &AnimeListModel::deleteRow);
  connect(anime::db(), &anime::Database::entryUpdated, this, &AnimeListModel::refreshRow);
  connect(anime::db(), &anime::Database::entryDeleted, this, &AnimeListModel::refreshRow);

  connect(taiga::settings(), &taiga::Settings::changed, this, [this](const QSet<QString>& keys) {
    if (keys.contains(taiga::Settings::Key::kAppTitleLanguage) && !ids_.isEmpty()) {
      emit dataChanged(index(0, 0), index(ids_.size() - 1, NumColumns - 1));
    }
  });
}

void AnimeListModel::refreshRow(int id) {
  if (const auto row = ids_.indexOf(id); row > -1) {
    emit dataChanged(index(row, 0), index(row, NumColumns - 1));
  } else {
    addIds({id});
  }
}

void AnimeListModel::deleteRow(int id) {
  if (const auto row = ids_.indexOf(id); row > -1) {
    beginRemoveRows({}, row, row);
    ids_.removeAt(row);
    endRemoveRows();
  }
}

void AnimeListModel::addIds(const QList<int>& ids) {
  QList<int> newIds;
  for (const int id : ids) {
    if (!ids_.contains(id)) newIds.append(id);
  }
  if (newIds.isEmpty()) return;

  const int first = ids_.size();
  beginInsertRows({}, first, first + newIds.size() - 1);
  ids_.append(newIds);
  endInsertRows();
}

int AnimeListModel::rowCount(const QModelIndex&) const {
  return ids_.size();
}

int AnimeListModel::columnCount(const QModelIndex&) const {
  return NumColumns;
}

QVariant AnimeListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return {};

  const auto anime = getAnime(index);
  if (!anime) return {};

  const auto entry = getListEntry(index);

  switch (role) {
    case Qt::DisplayRole:
      switch (index.column()) {
        case ColumnTitle:
          return QString::fromStdString(anime::preferredTitle(*anime));
        case ColumnDuration:
          return formatEpisodeLength(anime->episodeLength);
        case ColumnRewatches:
          if (entry) return entry->rewatchedTimes;
          break;
        case ColumnScore:
          if (entry) return formatRating(entry->score);
          break;
        case ColumnAverage:
          return formatScore(anime->score);
        case ColumnType:
          return formatType(anime->type);
        case ColumnSeason:
          return formatSeason(anime::Season(anime->dateStarted));
        case ColumnStarted:
          if (entry) return formatFuzzyDate(entry->dateStarted);
          break;
        case ColumnCompleted:
          if (entry) return formatFuzzyDate(entry->dateCompleted);
          break;
        case ColumnLastUpdated:
          if (entry) return formatAsRelativeTime(entry->lastUpdated, "-");
          break;
        case ColumnNotes:
          if (entry) return QString::fromStdString(entry->notes);
          break;
      }
      break;

    case Qt::ToolTipRole:
      switch (index.column()) {
        case ColumnTitle:
          return QString::fromStdString(anime::preferredTitle(*anime));
        case ColumnSeason:
          return formatFuzzyDate(anime->dateStarted);
        case ColumnLastUpdated:
          if (entry) return formatTimestamp(entry->lastUpdated);
          break;
        case ColumnNotes:
          if (entry) return QString::fromStdString(entry->notes);
          break;
      }
      break;

    case Qt::TextAlignmentRole: {
      switch (index.column()) {
        case ColumnProgress:
        case ColumnRewatches:
        case ColumnScore:
        case ColumnAverage:
        case ColumnType:
          return QVariant(Qt::AlignHCenter | Qt::AlignVCenter);
        case ColumnDuration:
        case ColumnSeason:
        case ColumnStarted:
        case ColumnCompleted:
        case ColumnLastUpdated:
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
        case ColumnAverage:
          if (!anime->score) return disabledTextColor;
          break;
        case ColumnDuration:
          if (anime->episodeLength < 1) return disabledTextColor;
          break;
        case ColumnSeason:
          if (!anime->dateStarted) return disabledTextColor;
          break;
        case ColumnType:
          if (anime->type == anime::Type::Unknown) return disabledTextColor;
          break;
        case ColumnScore:
          if (entry && !entry->score) return disabledTextColor;
          break;
        case ColumnStarted:
          if (entry && !entry->dateStarted) return disabledTextColor;
          break;
        case ColumnCompleted:
          if (entry && !entry->dateCompleted) return disabledTextColor;
          break;
        case ColumnLastUpdated:
          if (entry && !entry->lastUpdated) return disabledTextColor;
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
    case static_cast<int>(AnimeListItemDataRole::Poster): {
      return QVariant::fromValue(imageProvider()->loadPoster(anime->id));
    }
  }

  return {};
}

bool AnimeListModel::setData(const QModelIndex& index, const QVariant& value, int role) {
  if (index.isValid() && role == Qt::EditRole) {
    if (index.column() == ColumnScore) {
      const int id = ids_.at(index.row());
      if (const auto entry = anime::db()->entry(id)) {
        auto updated = *entry;
        updated.score = value.toInt();
        anime::list::save(updated);
      }
      return true;
    }
  }
  return false;
}

QVariant AnimeListModel::headerData(int section, Qt::Orientation orientation, int role) const {
  switch (role) {
    case Qt::DisplayRole: {
      // clang-format off
      switch (section) {
        case ColumnTitle: return tr("Title");
        case ColumnProgress: return tr("Progress");
        case ColumnDuration: return tr("Duration");
        case ColumnRewatches: return tr("Rewatches");
        case ColumnScore: return tr("Score");
        case ColumnAverage: return tr("Average");
        case ColumnType: return tr("Type");
        case ColumnSeason: return tr("Season");
        case ColumnStarted: return tr("Started");
        case ColumnCompleted: return tr("Completed");
        case ColumnLastUpdated: return tr("Last updated");
        case ColumnNotes: return tr("Notes");
      }
      // clang-format on
      break;
    }

    case Qt::TextAlignmentRole: {
      switch (section) {
        case ColumnProgress:
        case ColumnRewatches:
        case ColumnScore:
        case ColumnAverage:
        case ColumnType:
          return QVariant(Qt::AlignHCenter | Qt::AlignVCenter);
        case ColumnDuration:
        case ColumnSeason:
        case ColumnStarted:
        case ColumnCompleted:
        case ColumnLastUpdated:
          return QVariant(Qt::AlignRight | Qt::AlignVCenter);
      }
      break;
    }

    case Qt::InitialSortOrderRole: {
      switch (section) {
        case ColumnProgress:
        case ColumnDuration:
        case ColumnRewatches:
        case ColumnScore:
        case ColumnAverage:
        case ColumnSeason:
        case ColumnStarted:
        case ColumnCompleted:
        case ColumnLastUpdated:
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
  return anime::db()->item(ids_.at(index.row()));
}

const ListEntry* AnimeListModel::getListEntry(const QModelIndex& index) const {
  if (!index.isValid()) return nullptr;
  return anime::db()->entry(ids_.at(index.row()));
}

}  // namespace gui
