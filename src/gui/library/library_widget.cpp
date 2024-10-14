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

#include "library_widget.hpp"

#include <QDesktopServices>
#include <QHeaderView>
#include <QLayout>
#include <QUrl>

#include "gui/models/library_model.hpp"

namespace gui {

LibraryWidget::LibraryWidget(QWidget* parent)
    : QWidget{parent}, m_model(new LibraryModel(parent)), m_view(new QTreeView(parent)) {
  const auto rootPath = "C:/Media/Anime";  // @TODO

  m_model->setRootPath(rootPath);

  m_view->setObjectName("libraryView");
  m_view->setFrameShape(QFrame::Shape::NoFrame);
  m_view->setModel(m_model);
  m_view->setRootIndex(m_model->index(rootPath));
  m_view->setAlternatingRowColors(true);
  m_view->setAllColumnsShowFocus(true);
  m_view->setUniformRowHeights(true);

  m_view->header()->setSectionsMovable(false);
  m_view->header()->setStretchLastSection(false);
  m_view->header()->setTextElideMode(Qt::ElideRight);
  m_view->header()->hideSection(LibraryModel::COLUMN_TYPE);
  m_view->header()->resizeSection(LibraryModel::COLUMN_TITLE, 550);
  m_view->header()->resizeSection(LibraryModel::COLUMN_MODIFIED, 150);

  m_view->sortByColumn(LibraryModel::COLUMN_TITLE, Qt::SortOrder::AscendingOrder);
  m_view->setSortingEnabled(true);

  const auto layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  this->setLayout(layout);

  layout->addWidget(m_view);

  connect(m_view, &QTreeView::doubleClicked, this, [this](const QModelIndex& index) {
    if (!index.isValid()) return;
    const auto info = m_model->fileInfo(index);
    if (!info.isFile()) return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_model->filePath(index)));
  });
}

}  // namespace gui
