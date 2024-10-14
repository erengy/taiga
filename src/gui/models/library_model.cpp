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

#include <anitomy.hpp>
#include <anitomy/detail/keyword.hpp>  // don't try this at home
#include <ranges>

namespace gui {

LibraryModel::LibraryModel(QObject* parent) : QFileSystemModel(parent) {
  setNameFilters([]() {
    QStringList filters;
    for (const auto& [key, keyword] : anitomy::detail::keywords) {
      if (keyword.kind != anitomy::detail::KeywordKind::FileExtension) continue;
      filters.emplace_back(u"*.%1"_qs.arg(QString::fromStdString(key.data())));
    }
    return filters;
  }());
  setNameFilterDisables(true);

  connect(this, &QFileSystemModel::directoryLoaded, this, &LibraryModel::parseDirectory);
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
          if (isEnabled(index)) return getTitle(fileName(index));
          break;
        case COLUMN_EPISODE:
          if (isEnabled(index)) return getEpisode(fileName(index));
          break;
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
      switch (section) {
        case COLUMN_NAME:
          return tr("Name");
        case COLUMN_SIZE:
          return tr("Size");
        case COLUMN_TYPE:
          return tr("Type");
        case COLUMN_ANIME:
          return tr("Anime");
        case COLUMN_EPISODE:
          return tr("Episode");
        case COLUMN_MODIFIED:
          return tr("Last modified");
      }
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
  return m_parsed[path].title;
}

QString LibraryModel::getEpisode(const QString& path) const {
  return m_parsed[path].episode;
}

void LibraryModel::parseDirectory(const QString& path) {
  const auto parent = index(path);

  if (!parent.isValid()) return;

  for (int i = 0; i < rowCount(parent); ++i) {
    const auto child = index(i, 0, parent);
    if (!child.isValid()) continue;
    if (!isEnabled(child)) continue;
    const auto info = fileInfo(child);
    if (!info.isFile()) continue;
    parseFileName(info.fileName());
  }
}

void LibraryModel::parseFileName(const QString& name) {
  if (m_parsed.contains(name)) return;

  const auto elements = anitomy::parse(name.toStdString());

  const auto find_element = [&elements](anitomy::ElementKind kind) {
    return std::ranges::find_if(
        elements, [kind](const anitomy::Element& element) { return element.kind == kind; });
  };

  const auto title = find_element(anitomy::ElementKind::Title);
  const auto episode = find_element(anitomy::ElementKind::Episode);

  m_parsed[name] = ParsedData{
      .title = title != elements.end() ? QString::fromStdString(title->value) : "",
      .episode = episode != elements.end() ? QString::fromStdString(episode->value) : "",
  };
}

}  // namespace gui
