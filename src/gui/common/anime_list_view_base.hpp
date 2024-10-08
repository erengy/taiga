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

#pragma once

#include <QAbstractItemView>
#include <QObject>
#include <QTreeWidget>

namespace gui {

class AnimeListModel;
class AnimeListProxyModel;
class MainWindow;

class ListViewBase final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(ListViewBase)

public:
  ListViewBase(QWidget* parent, QAbstractItemView* view, AnimeListModel* model,
               AnimeListProxyModel* proxyModel, MainWindow* mainWindow);
  ~ListViewBase() = default;

private slots:
  void filterByListStatus(QTreeWidgetItem* current);
  void filterByText(const QString& text);
  void showMediaDialog(const QModelIndex& index);
  void showMediaMenu();
  void updateSelectionStatus(const QItemSelection& selected, const QItemSelection& deselected);

private:
  QModelIndexList selectedIndexes();

  AnimeListModel* m_model = nullptr;
  AnimeListProxyModel* m_proxyModel = nullptr;
  MainWindow* m_mainWindow = nullptr;
  QAbstractItemView* m_view = nullptr;
};

}  // namespace gui
