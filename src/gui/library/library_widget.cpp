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

#include "library_widget.hpp"

#include <QHeaderView>
#include <QKeyEvent>
#include <QLayout>
#include <QSignalBlocker>
#include <algorithm>

#include "base/string.hpp"
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
  m_model->setReadOnly(false);

  auto filtersLayout = new QHBoxLayout();
  filtersLayout->setSpacing(4);
  m_toolbarLayout->insertLayout(0, filtersLayout);

  // Root
  {
    m_comboRoot->setPlaceholderText("Location");
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
    const auto actionMore = new QAction(theme()->getIcon("more_horiz"), tr("More"), this);
    m_toolbar->addAction(actionMore);
  }

  m_view->installEventFilter(this);
  m_view->setObjectName("libraryView");
  m_view->setFrameShape(QFrame::Shape::NoFrame);
  m_view->setModel(m_model);
  m_view->setAlternatingRowColors(true);
  m_view->setAllColumnsShowFocus(true);
  m_view->setContextMenuPolicy(Qt::CustomContextMenu);
  m_view->setEditTriggers(QAbstractItemView::EditKeyPressed);
  m_view->setUniformRowHeights(true);

  m_view->setDragEnabled(true);
  m_view->setAcceptDrops(true);
  m_view->setDropIndicatorShown(true);
  m_view->setDragDropMode(QAbstractItemView::InternalMove);

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
    if (!m_model->fileInfo(index).isFile()) return;  // let double-click expand/collapse folders
    LibraryMenu::openPath(m_model->filePath(index));
  });

  refreshFolders();
  connect(taiga::settings(), &taiga::Settings::changed, this, [this](const QSet<QString>& keys) {
    if (keys.contains(u"library.folders"_s)) refreshFolders();
  });
}

bool LibraryWidget::eventFilter(QObject* watched, QEvent* event) {
  if (watched == m_view && event->type() == QEvent::KeyPress) {
    const auto* keyEvent = static_cast<QKeyEvent*>(event);
    const auto index = m_view->currentIndex();

    if (index.isValid()) {
      const auto path = m_model->fileInfo(index).filePath();

      switch (keyEvent->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
          LibraryMenu::openPath(path);
          return true;
        case Qt::Key_Delete:
          LibraryMenu::deletePath(m_view, path);
          return true;
      }
    }
  }

  return PageWidget::eventFilter(watched, event);
}

void LibraryWidget::refreshFolders() {
  const auto folders = taiga::settings()->libraryFolders();

  {
    // Keep the current folder if it's still there.
    const auto current = m_comboRoot->currentText();
    const QSignalBlocker blocker{m_comboRoot};
    m_comboRoot->clear();
    for (const auto& folder : folders) {
      m_comboRoot->addItem(QString::fromStdString(folder));
    }
    m_comboRoot->setCurrentIndex(folders.empty() ? -1
                                                 : std::max(0, m_comboRoot->findText(current)));
  }

  m_comboRoot->setDisabled(folders.empty());
  m_view->setRootIndex(m_model->setRootPath(m_comboRoot->currentText()));
}

void LibraryWidget::showContextMenu() const {
  const auto index = m_view->currentIndex();

  if (!index.isValid()) return;

  const QString path = m_model->fileInfo(index).filePath();

  auto* menu = new LibraryMenu(m_view, path, m_model->getId(path));
  menu->popup();
}

}  // namespace gui
