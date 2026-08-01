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

#include "history_widget.hpp"

#include <QDesktopServices>
#include <QHeaderView>
#include <QLayout>
#include <QMenu>
#include <QMessageBox>
#include <QUrl>

#include "gui/main/main_window.hpp"
#include "gui/media/media_dialog.hpp"
#include "gui/models/history_model.hpp"
#include "gui/models/history_proxy_model.hpp"
#include "gui/utils/theme.hpp"
#include "media/anime_db.hpp"
#include "media/anime_history.hpp"
#include "taiga/settings.hpp"

namespace gui {

HistoryWidget::HistoryWidget(QWidget* parent)
    : PageWidget{parent},
      m_model(new HistoryModel(parent)),
      m_proxyModel(new HistoryProxyModel(parent)),
      m_view(new QTreeView(parent)) {
  m_proxyModel->setSourceModel(m_model);
  m_proxyModel->sort(HistoryModel::COLUMN_MODIFIED, Qt::SortOrder::DescendingOrder);

  m_view->setObjectName("historyView");
  m_view->setFrameShape(QFrame::Shape::NoFrame);
  m_view->setModel(m_proxyModel);
  m_view->setAlternatingRowColors(true);
  m_view->setAllColumnsShowFocus(true);
  m_view->setContextMenuPolicy(Qt::CustomContextMenu);
  m_view->setRootIsDecorated(false);
  m_view->setUniformRowHeights(true);

  m_view->header()->setSectionsMovable(false);
  m_view->header()->setStretchLastSection(false);
  m_view->header()->setTextElideMode(Qt::ElideRight);
  m_view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  m_view->header()->setSectionResizeMode(HistoryModel::COLUMN_TITLE, QHeaderView::Stretch);
  m_view->header()->setSectionResizeMode(HistoryModel::COLUMN_DETAILS, QHeaderView::Stretch);

  layout()->addWidget(m_view);

  connect(m_view, &QWidget::customContextMenuRequested, this, &HistoryWidget::showContextMenu);
  connect(m_view, &QTreeView::doubleClicked, this, &HistoryWidget::showMediaDialog);
}

void HistoryWidget::showContextMenu() const {
  const auto index = m_view->currentIndex();

  if (!index.isValid()) return;

  auto* menu = new QMenu(m_view);
  menu->setAttribute(Qt::WA_DeleteOnClose);

  menu->addAction(theme.getIcon("info"), tr("Details"), this,
                  [this, index]() { showMediaDialog(index); });
  menu->addAction(theme.getIcon("delete"), tr("Remove..."), this,
                  [this, index]() { removeItem(index); });

  menu->popup(QCursor::pos());
}

void HistoryWidget::showMediaDialog(const QModelIndex& index) const {
  const auto historyItem = getHistoryItem(index);
  if (!historyItem) return;

  const auto item = anime::db.item(historyItem->anime_id);
  if (!item) return;

  const auto entry = anime::db.entry(historyItem->anime_id);
  MediaDialog::show(mainWindow(), MediaDialogPage::Details, *item,
                    entry ? std::optional<ListEntry>{*entry} : std::nullopt);
}

void HistoryWidget::removeItem(const QModelIndex& index) const {
  const auto historyItem = getHistoryItem(index);
  if (!historyItem) return;

  const auto item = anime::db.item(historyItem->anime_id);
  const auto title = item ? QString::fromStdString(item->titles.romaji) : tr("Unknown");

  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Icon::Question);
  msgBox.setText(tr("Do you want to remove this entry from history?"));
  msgBox.setInformativeText(tr("%1 - Episode %2").arg(title).arg(historyItem->episode));

  auto removeButton = msgBox.addButton(tr("Remove"), QMessageBox::ButtonRole::DestructiveRole);
  msgBox.addButton(QMessageBox::Cancel);
  msgBox.setDefaultButton(QMessageBox::Cancel);

  msgBox.exec();

  if (msgBox.clickedButton() == reinterpret_cast<QAbstractButton*>(removeButton)) {
    anime::history.remove(historyItem->id);
  }
}

const anime::HistoryItem* HistoryWidget::getHistoryItem(const QModelIndex& index) const {
  const int role = static_cast<int>(HistoryItemDataRole::HistoryItem);
  return index.data(role).value<const anime::HistoryItem*>();
}

}  // namespace gui
