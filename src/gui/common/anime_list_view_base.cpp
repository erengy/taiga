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

#include "anime_list_view_base.hpp"

#include <QLineEdit>
#include <QListView>
#include <QStatusBar>
#include <QTreeView>

#include "gui/main/main_window.hpp"
#include "gui/main/navigation_item_delegate.hpp"
#include "gui/main/navigation_widget.hpp"
#include "gui/main/now_playing_widget.hpp"
#include "gui/media/media_dialog.hpp"
#include "gui/media/media_menu.hpp"
#include "gui/models/anime_list_model.hpp"
#include "gui/models/anime_list_proxy_model.hpp"
#include "media/anime.hpp"

namespace gui {

ListViewBase::ListViewBase(QWidget* parent, QAbstractItemView* view, AnimeListModel* model,
                           AnimeListProxyModel* proxyModel, MainWindow* mainWindow)
    : QObject(parent),
      m_view(view),
      m_model(model),
      m_proxyModel(proxyModel),
      m_mainWindow(mainWindow) {
  m_view->setContextMenuPolicy(Qt::CustomContextMenu);
  m_view->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

  m_proxyModel->setSourceModel(m_model);
  m_view->setModel(m_proxyModel);

  connect(mainWindow->searchBox(), &QLineEdit::textChanged, this, &ListViewBase::filterByText);

  connect(mainWindow->navigation(), &NavigationWidget::currentItemChanged, this,
          &ListViewBase::filterByListStatus);

  connect(m_view, &QAbstractItemView::doubleClicked, this, &ListViewBase::showMediaDialog);

  connect(m_view, &QWidget::customContextMenuRequested, this, &ListViewBase::showMediaMenu);

  connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &ListViewBase::updateSelectionStatus);
}

void ListViewBase::filterByListStatus(QTreeWidgetItem* current) {
  if (!current) return;
  const int role = static_cast<int>(NavigationItemDataRole::ListStatus);
  if (const int status = current->data(0, role).toInt()) {
    m_proxyModel->setListStatusFilter(status);
  } else {
    m_proxyModel->removeListStatusFilter();
  }
}

void ListViewBase::filterByText(const QString& text) {
  m_proxyModel->setTextFilter(text);
}

void ListViewBase::showMediaDialog(const QModelIndex& index) {
  const auto mappedIndex = m_proxyModel->mapToSource(index);
  const auto anime = m_model->getAnime(mappedIndex);
  if (!anime) return;
  const auto entry = m_model->getListEntry(mappedIndex);
  MediaDialog::show(m_view, *anime, entry ? std::optional<ListEntry>{*entry} : std::nullopt);
}

void ListViewBase::showMediaMenu() {
  const auto indexes = selectedIndexes();
  if (indexes.isEmpty()) return;

  QList<Anime> items;
  QMap<int, ListEntry> entries;

  for (auto selectedIndex : indexes) {
    const auto index = m_proxyModel->mapToSource(selectedIndex);
    if (const auto item = m_model->getAnime(index)) {
      items.push_back(*item);
      if (const auto entry = m_model->getListEntry(index)) {
        entries[item->id] = *entry;
      }
    }
  }

  auto* menu = new MediaMenu(m_view, items, entries);
  menu->popup();
}

void ListViewBase::updateSelectionStatus(const QItemSelection& selected,
                                         const QItemSelection& deselected) {
  // @TEMP
  if (!selected.empty()) {
    const auto selectedItem =
        m_model->getAnime(m_proxyModel->mapToSource(selected.indexes().first()));
    m_mainWindow->nowPlaying()->setPlaying(*selectedItem);
    m_mainWindow->nowPlaying()->show();
  } else if (m_mainWindow && m_mainWindow->nowPlaying()) {
    m_mainWindow->nowPlaying()->hide();
  }

  if (const auto n = selectedIndexes().size()) {
    m_mainWindow->statusBar()->showMessage(tr("%n item(s) selected", nullptr, n));
  } else {
    m_mainWindow->statusBar()->clearMessage();
  }
}

QModelIndexList ListViewBase::selectedIndexes() {
  const auto model = m_view->selectionModel();
  return model->selectedRows().size() ? model->selectedRows() : model->selectedIndexes();
}

}  // namespace gui
