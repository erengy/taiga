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

#include "library_model.hpp"

#include <QApplication>
#include <QFileInfo>
#include <QPalette>
#include <algorithm>
#include <anitomy.hpp>
#include <anitomy/detail/keyword.hpp>  // don't try this at home
#include <ranges>

#include "base/file.hpp"
#include "media/anime_db.hpp"
#include "media/anime_utils.hpp"
#include "track/episode.hpp"
#include "track/recognition.hpp"

namespace gui {

LibraryModel::LibraryModel(QObject* parent) : QFileSystemModel(parent) {}

int LibraryModel::columnCount(const QModelIndex&) const {
  return NumColumns;
}

QVariant LibraryModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return {};

  switch (role) {
    case Qt::DisplayRole: {
      switch (index.column()) {
        case ColumnAnime:
          if (isVideoFile(index)) return getTitle(filePath(index));
          return {};
        case ColumnEpisode:
          if (isVideoFile(index)) return getEpisode(filePath(index));
          return {};
      }
      break;
    }

    case Qt::ForegroundRole: {
      switch (index.column()) {
        case ColumnName: {
          const auto info = fileInfo(index);
          if (base::isExecutableFile(filePath(index))) {
            return QColorConstants::Red;  // potentially dangerous file
          }
          if (info.isFile() && !isVideoFile(index)) {
            return qApp->palette().color(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Text);
          }
          break;
        }
        case ColumnAnime: {
          const auto disabledTextColor =
              qApp->palette().color(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Text);
          if (!getId(filePath(index))) return disabledTextColor;  // unidentified
          break;
        }
      }
      break;
    }

    case Qt::TextAlignmentRole: {
      switch (index.column()) {
        case ColumnSize:
        case ColumnModified:
        case ColumnEpisode:
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
      // clang-format off
      switch (section) {
        case ColumnName: return tr("Name");
        case ColumnSize: return tr("Size");
        case ColumnType: return tr("Type");
        case ColumnAnime: return tr("Anime");
        case ColumnEpisode: return tr("Episode");
        case ColumnModified: return tr("Last modified");
      }
      // clang-format on
      break;
    }

    case Qt::TextAlignmentRole: {
      switch (section) {
        case ColumnName:
        case ColumnType:
        case ColumnAnime:
          return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        case ColumnSize:
        case ColumnModified:
        case ColumnEpisode:
          return QVariant(Qt::AlignRight | Qt::AlignVCenter);
      }
      break;
    }

    case Qt::InitialSortOrderRole: {
      switch (section) {
        case ColumnSize:
        case ColumnModified:
          return Qt::DescendingOrder;
        default:
          return Qt::AscendingOrder;
      }
      break;
    }
  }

  return QFileSystemModel::headerData(section, orientation, role);
}

bool LibraryModel::isVideoFile(const QModelIndex& index) const {
  const auto info = fileInfo(index);
  if (!info.isFile()) return false;

  return std::ranges::contains(anitomy::detail::file_extensions,
                               info.suffix().toLower().toStdString());
}

QString LibraryModel::getTitle(const QString& path) const {
  parse(path);

  if (parsed_[path].id) {
    const auto item = anime::db()->item(parsed_[path].id);
    if (item) return QString::fromStdString(anime::preferredTitle(*item));
  }

  return parsed_[path].title;
}

QString LibraryModel::getEpisode(const QString& path) const {
  parse(path);
  return parsed_[path].episode;
}

int LibraryModel::getId(const QString& path) const {
  parse(path);
  return parsed_[path].id;
}

void LibraryModel::parse(const QString& path) const {
  if (parsed_.contains(path)) return;

  const auto info = fileInfo(index(path));
  if (!info.isFile()) return;

  auto episode = track::recognition::parseFileInfo(info);
  const auto animeId = track::recognition::identify(episode);

  parsed_[path] = ParsedData{
      .title = QString::fromStdString(episode.element(anitomy::ElementKind::Title)),
      .episode = QString::fromStdString(episode.element(anitomy::ElementKind::Episode)),
      .id = animeId,
  };
}

}  // namespace gui
