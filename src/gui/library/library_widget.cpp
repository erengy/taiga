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

#include "gui/library/library_menu.hpp"
#include "gui/main/main_window.hpp"
#include "gui/models/library_model.hpp"
#include "gui/utils/theme.hpp"
#include "taiga/settings.hpp"
#include "ui_main_window.h"

namespace gui {

LibraryWidget::LibraryWidget(QWidget* parent)
    : PageWidget{parent},
      m_model(new LibraryModel(parent)),
      m_comboRoot(new ComboBox(this)),
      m_view(new QTreeView(parent)) {
  const auto libraryFolders = taiga::settings.libraryFolders();
  const auto rootPath =
      !libraryFolders.empty() ? QString::fromStdString(libraryFolders.front()) : QString{};

  m_model->setRootPath(rootPath);

  auto filtersLayout = new QHBoxLayout();
  filtersLayout->setSpacing(4);
  m_toolbarLayout->insertLayout(0, filtersLayout);

  // Root
  {
    m_comboRoot->setPlaceholderText("Location");
    m_comboRoot->setDisabled(rootPath.isEmpty());
    for (const auto& folder : libraryFolders) {
      m_comboRoot->addItem(QString::fromStdString(folder));
    }
    m_comboRoot->setCurrentText(rootPath);
    connect(m_comboRoot, &QComboBox::currentIndexChanged, this,
            [this](int index) { m_model->setRootPath(m_comboRoot->itemText(index)); });
    filtersLayout->addWidget(m_comboRoot);
  }

  // Toolbar
  {
    // Play next episode
    m_toolbar->addAction(mainWindow()->ui()->actionPlayNextEpisode);

    // Play random anime
    m_toolbar->addAction(mainWindow()->ui()->actionPlayRandomAnime);

    // More
    const auto actionMore = new QAction(theme.getIcon("more_horiz"), tr("More"), this);
    m_toolbar->addAction(actionMore);
  }

  m_view->setObjectName("libraryView");
  m_view->setFrameShape(QFrame::Shape::NoFrame);
  m_view->setModel(m_model);
  m_view->setRootIndex(m_model->index(rootPath));
  m_view->setAlternatingRowColors(true);
  m_view->setAllColumnsShowFocus(true);
  m_view->setContextMenuPolicy(Qt::CustomContextMenu);
  m_view->setUniformRowHeights(true);

  m_view->header()->setSectionsMovable(false);
  m_view->header()->setStretchLastSection(false);
  m_view->header()->setTextElideMode(Qt::ElideRight);
  m_view->header()->hideSection(LibraryModel::COLUMN_TYPE);
  m_view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  m_view->header()->setSectionResizeMode(LibraryModel::COLUMN_NAME, QHeaderView::Stretch);
  m_view->header()->setSectionResizeMode(LibraryModel::COLUMN_ANIME, QHeaderView::Stretch);
  m_view->header()->moveSection(LibraryModel::COLUMN_ANIME, 1);
  m_view->header()->moveSection(LibraryModel::COLUMN_EPISODE, 2);

  m_view->sortByColumn(LibraryModel::COLUMN_NAME, Qt::SortOrder::AscendingOrder);
  m_view->setSortingEnabled(true);

  layout()->addWidget(m_view);

  connect(m_model, &QFileSystemModel::rootPathChanged, this,
          [this](const QString& newPath) { m_view->setRootIndex(m_model->index(newPath)); });

  connect(m_view, &QWidget::customContextMenuRequested, this, &LibraryWidget::showContextMenu);

  connect(m_view, &QTreeView::doubleClicked, this, [this](const QModelIndex& index) {
    if (!index.isValid()) return;
    if (!(index.flags() & Qt::ItemIsEnabled)) return;
    const auto info = m_model->fileInfo(index);
    if (!info.isFile()) return;
    if (info.isExecutable()) return;  // avoid running potentially dangerous files
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_model->filePath(index)));
  });
}

void LibraryWidget::showContextMenu() const {
  const auto index = m_view->currentIndex();

  if (!index.isValid()) return;

  const QString path = m_model->fileInfo(index).filePath();

  auto* menu = new LibraryMenu(m_view, path, m_model->getId(path));
  menu->popup();
}

}  // namespace gui
