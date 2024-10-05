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

#include "list_view.hpp"

#include <QHeaderView>
#include <QLineEdit>
#include <QStatusBar>

#include "gui/list/list_item_delegate.hpp"
#include "gui/list/list_model.hpp"
#include "gui/list/list_proxy_model.hpp"
#include "gui/main/main_window.hpp"
#include "gui/main/navigation_item_delegate.hpp"
#include "gui/main/navigation_widget.hpp"
#include "gui/main/now_playing_widget.hpp"
#include "gui/media/media_dialog.hpp"
#include "gui/media/media_menu.hpp"
#include "gui/utils/theme.hpp"
#include "media/anime.hpp"

namespace gui {

ListView::ListView(QWidget* parent, MainWindow* mainWindow)
    : QTreeView(parent),
      m_model(new ListModel(this)),
      m_proxyModel(new ListProxyModel(this)),
      m_mainWindow(mainWindow) {
  setObjectName("animeList");

  setItemDelegate(new ListItemDelegate(this));

  m_proxyModel->setSourceModel(m_model);
  setModel(m_proxyModel);

  setAllColumnsShowFocus(true);
  setAlternatingRowColors(true);
  setContextMenuPolicy(Qt::CustomContextMenu);
  setExpandsOnDoubleClick(false);
  setFrameShape(QFrame::Shape::NoFrame);
  setItemsExpandable(false);
  setRootIsDecorated(false);
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  setSortingEnabled(true);
  setUniformRowHeights(true);

  sortByColumn(ListModel::COLUMN_SEASON, Qt::SortOrder::DescendingOrder);

  if (theme.isDark()) {
    setPalette([this]() {
      QPalette palette = this->palette();
      auto alternateColor = palette.color(QPalette::Base).darker(150);
      palette.setColor(QPalette::AlternateBase, alternateColor);
      return palette;
    }());
  }

  header()->setFirstSectionMovable(true);
  header()->setStretchLastSection(false);
  header()->setTextElideMode(Qt::ElideRight);
  header()->resizeSection(ListModel::COLUMN_TITLE, 300);
  header()->resizeSection(ListModel::COLUMN_PROGRESS, 150);

  connect(mainWindow->searchBox(), &QLineEdit::textChanged, this,
          [this](const QString& text) { m_proxyModel->setTextFilter(text); });

  connect(mainWindow->navigation(), &NavigationWidget::currentItemChanged, this,
          [this](QTreeWidgetItem* current, QTreeWidgetItem* previous) {
            if (!current) return;
            const int role = static_cast<int>(NavigationItemDataRole::ListStatus);
            if (const int status = current->data(0, role).toInt()) {
              m_proxyModel->setListStatusFilter(status);
            } else {
              m_proxyModel->removeListStatusFilter();
            }
          });

  connect(this, &QWidget::customContextMenuRequested, this, [this]() {
    const auto selectedRows = selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) return;

    QList<Anime> items;
    for (auto selectedIndex : selectedRows) {
      if (const auto item = m_model->getAnime(m_proxyModel->mapToSource(selectedIndex))) {
        items.push_back(*item);
      }
    }

    auto* menu = new MediaMenu(this, items);
    menu->popup();
  });

  connect(this, &QAbstractItemView::clicked, this,
          qOverload<const QModelIndex&>(&QAbstractItemView::edit));

  connect(this, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex& index) {
    const auto anime = m_model->getAnime(m_proxyModel->mapToSource(index));
    if (!anime) return;
    const auto entry = m_model->getListEntry(m_proxyModel->mapToSource(index));
    MediaDialog::show(this, *anime, entry ? std::optional<ListEntry>{*entry} : std::nullopt);
  });

  connect(selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &ListView::selectionChanged);
}

void ListView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
  QTreeView::selectionChanged(selected, deselected);

  if (!selected.empty()) {
    const auto selectedItem =
        m_model->getAnime(m_proxyModel->mapToSource(selected.indexes().first()));
    m_mainWindow->nowPlaying()->setPlaying(*selectedItem);
    m_mainWindow->nowPlaying()->show();
  } else if (m_mainWindow && m_mainWindow->nowPlaying()) {
    m_mainWindow->nowPlaying()->hide();
  }

  if (const auto n = selectionModel()->selectedRows().size()) {
    m_mainWindow->statusBar()->showMessage(tr("%n item(s) selected", nullptr, n));
  } else {
    m_mainWindow->statusBar()->clearMessage();
  }
}

}  // namespace gui
