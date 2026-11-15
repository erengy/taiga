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

#include "gui/common/header_view.hpp"
#include "gui/library/library_menu.hpp"
#include "gui/main/main_window.hpp"
#include "gui/models/library_model.hpp"
#include "gui/utils/theme.hpp"
#include "taiga/settings.hpp"
#include "ui_main_window.h"

namespace gui {

LibraryWidget::LibraryWidget(QWidget* parent)
    : PageWidget{parent},
      model_(new LibraryModel(parent)),
      comboRoot_(new ComboBox(this)),
      view_(new QTreeView(parent)) {
  model_->setReadOnly(false);

  auto filtersLayout = new QHBoxLayout();
  filtersLayout->setSpacing(4);
  toolbarLayout_->insertLayout(0, filtersLayout);

  // Root
  {
    comboRoot_->setPlaceholderText("Location");
    connect(comboRoot_, &QComboBox::currentIndexChanged, this,
            [this](int index) { model_->setRootPath(comboRoot_->itemText(index)); });
    filtersLayout->addWidget(comboRoot_);
  }

  // Toolbar
  {
    // Play next episode
    toolbar_->addAction(mainWindow()->ui()->actionPlayNextEpisode);

    // Play random anime
    toolbar_->addAction(mainWindow()->ui()->actionPlayRandomAnime);

    // More
    const auto actionMore = new QAction(theme()->getIcon("more_horiz"), tr("More"), this);
    toolbar_->addAction(actionMore);
  }

  view_->installEventFilter(this);
  view_->setObjectName("libraryView");
  view_->setProperty("themed", true);
  view_->setFrameShape(QFrame::Shape::NoFrame);
  view_->setModel(model_);
  view_->setAlternatingRowColors(true);
  view_->setAllColumnsShowFocus(true);
  view_->setContextMenuPolicy(Qt::CustomContextMenu);
  view_->setEditTriggers(QAbstractItemView::EditKeyPressed);
  view_->setUniformRowHeights(true);

  view_->setDragEnabled(true);
  view_->setAcceptDrops(true);
  view_->setDropIndicatorShown(true);
  view_->setDragDropMode(QAbstractItemView::InternalMove);

  HeaderView::install(view_);
  view_->header()->setSectionsMovable(false);
  view_->header()->setStretchLastSection(false);
  view_->header()->setTextElideMode(Qt::ElideRight);
  view_->header()->hideSection(LibraryModel::ColumnType);
  view_->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  view_->header()->setSectionResizeMode(LibraryModel::ColumnName, QHeaderView::Stretch);
  view_->header()->setSectionResizeMode(LibraryModel::ColumnAnime, QHeaderView::Stretch);
  view_->header()->moveSection(LibraryModel::ColumnAnime, 1);
  view_->header()->moveSection(LibraryModel::ColumnEpisode, 2);

  view_->sortByColumn(LibraryModel::ColumnName, Qt::SortOrder::AscendingOrder);
  view_->setSortingEnabled(true);

  layout()->addWidget(view_);

  connect(model_, &QFileSystemModel::rootPathChanged, this,
          [this](const QString& newPath) { view_->setRootIndex(model_->index(newPath)); });

  connect(view_, &QWidget::customContextMenuRequested, this, &LibraryWidget::showContextMenu);

  connect(view_, &QTreeView::doubleClicked, this, [this](const QModelIndex& index) {
    if (!index.isValid()) return;
    if (!model_->fileInfo(index).isFile()) return;  // let double-click expand/collapse folders
    LibraryMenu::openPath(model_->filePath(index));
  });

  refreshFolders();
  connect(taiga::settings(), &taiga::Settings::changed, this, [this](const QSet<QString>& keys) {
    if (keys.contains(taiga::Settings::Key::kLibraryFolders)) refreshFolders();
  });
}

bool LibraryWidget::eventFilter(QObject* watched, QEvent* event) {
  if (watched == view_ && event->type() == QEvent::KeyPress) {
    const auto* keyEvent = static_cast<QKeyEvent*>(event);
    const auto index = view_->currentIndex();

    if (index.isValid()) {
      const auto path = model_->fileInfo(index).filePath();

      switch (keyEvent->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
          LibraryMenu::openPath(path);
          return true;
        case Qt::Key_Delete:
          LibraryMenu::deletePath(view_, path);
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
    const auto current = comboRoot_->currentText();
    const QSignalBlocker blocker{comboRoot_};
    comboRoot_->clear();
    for (const auto& folder : folders) {
      comboRoot_->addItem(QString::fromStdString(folder));
    }
    comboRoot_->setCurrentIndex(folders.empty() ? -1 : std::max(0, comboRoot_->findText(current)));
  }

  comboRoot_->setDisabled(folders.empty());
  view_->setRootIndex(model_->setRootPath(comboRoot_->currentText()));
}

void LibraryWidget::showContextMenu() const {
  const auto index = view_->currentIndex();

  if (!index.isValid()) return;

  const QString path = model_->fileInfo(index).filePath();

  auto* menu = new LibraryMenu(view_, path, model_->getId(path));
  menu->popup();
}

}  // namespace gui
