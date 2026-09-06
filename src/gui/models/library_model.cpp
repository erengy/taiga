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
#include <anitomy.hpp>
#include <anitomy/detail/keyword.hpp>  // don't try this at home
#include <ranges>

#include "base/string.hpp"
#include "media/anime_db.hpp"
#include "media/anime_utils.hpp"
#include "track/episode.hpp"
#include "track/recognition.hpp"

namespace gui {

LibraryModel::LibraryModel(QObject* parent) : QFileSystemModel(parent) {
  setNameFilters([]() {
    QStringList filters;
    for (const auto& extension : anitomy::detail::file_extensions) {
      filters.emplace_back(u"*.%1"_s.arg(extension));
    }
    return filters;
  }());
  setNameFilterDisables(true);
}

int LibraryModel::columnCount(const QModelIndex&) const {
  return NUM_COLUMNS;
}

QVariant LibraryModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return {};

  switch (role) {
    case Qt::DisplayRole: {
      switch (index.column()) {
        case COLUMN_ANIME:
          if (isEnabled(index)) return getTitle(filePath(index));
          return {};
        case COLUMN_EPISODE:
          if (isEnabled(index)) return getEpisode(filePath(index));
          return {};
      }
      break;
    }

    case Qt::ForegroundRole: {
      switch (index.column()) {
        case COLUMN_NAME: {
          const auto info = fileInfo(index);
          if (info.isFile() && info.isExecutable()) {
            return QColorConstants::Red;  // potentially dangerous file
          }
          break;
        }
        case COLUMN_ANIME: {
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
        case COLUMN_SIZE:
        case COLUMN_MODIFIED:
        case COLUMN_EPISODE:
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
        case COLUMN_NAME: return tr("Name");
        case COLUMN_SIZE: return tr("Size");
        case COLUMN_TYPE: return tr("Type");
        case COLUMN_ANIME: return tr("Anime");
        case COLUMN_EPISODE: return tr("Episode");
        case COLUMN_MODIFIED: return tr("Last modified");
      }
      // clang-format on
      break;
    }

    case Qt::TextAlignmentRole: {
      switch (section) {
        case COLUMN_NAME:
        case COLUMN_TYPE:
        case COLUMN_ANIME:
          return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        case COLUMN_SIZE:
        case COLUMN_MODIFIED:
        case COLUMN_EPISODE:
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

bool LibraryModel::isEnabled(const QModelIndex& index) const {
  return index.flags() & Qt::ItemIsEnabled;
}

QString LibraryModel::getTitle(const QString& path) const {
  parse(path);

  if (m_parsed[path].id) {
    const auto item = anime::db.item(m_parsed[path].id);
    if (item) return QString::fromStdString(anime::preferredTitle(*item));
  }

  return m_parsed[path].title;
}

QString LibraryModel::getEpisode(const QString& path) const {
  parse(path);
  return m_parsed[path].episode;
}

int LibraryModel::getId(const QString& path) const {
  parse(path);
  return m_parsed[path].id;
}

void LibraryModel::parse(const QString& path) const {
  if (m_parsed.contains(path)) return;

  const auto info = fileInfo(index(path));
  if (!info.isFile()) return;

  auto episode = track::recognition::parseFileInfo(info);
  const auto anime_id = track::recognition::identify(episode);

  m_parsed[path] = ParsedData{
      .title = QString::fromStdString(episode.element(anitomy::ElementKind::Title)),
      .episode = QString::fromStdString(episode.element(anitomy::ElementKind::Episode)),
      .id = anime_id,
  };
}

}  // namespace gui
