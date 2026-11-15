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
#include <QToolButton>
#include <QUrl>

#include "gui/common/header_view.hpp"
#include "gui/main/main_window.hpp"
#include "gui/media/media_dialog.hpp"
#include "gui/models/history_model.hpp"
#include "gui/models/history_proxy_model.hpp"
#include "gui/utils/theme.hpp"
#include "gui/utils/widgets.hpp"
#include "media/anime_db.hpp"
#include "media/anime_history.hpp"
#include "media/anime_utils.hpp"
#include "sync/queue.hpp"
#include "sync/service.hpp"
#include "taiga/settings.hpp"

namespace gui {

HistoryWidget::HistoryWidget(QWidget* parent)
    : PageWidget{parent},
      model_(new HistoryModel(parent)),
      proxyModel_(new HistoryProxyModel(parent)),
      view_(new QTreeView(parent)) {
  proxyModel_->setSourceModel(model_);
  proxyModel_->sort(HistoryModel::ColumnModified, Qt::SortOrder::DescendingOrder);

  view_->setObjectName("historyView");
  view_->setProperty("themed", true);
  view_->setFrameShape(QFrame::Shape::NoFrame);
  view_->setModel(proxyModel_);
  view_->setAlternatingRowColors(true);
  view_->setAllColumnsShowFocus(true);
  view_->setContextMenuPolicy(Qt::CustomContextMenu);
  view_->setRootIsDecorated(false);
  view_->setUniformRowHeights(true);

  HeaderView::install(view_);
  view_->header()->setSectionsMovable(false);
  view_->header()->setStretchLastSection(false);
  view_->header()->setTextElideMode(Qt::ElideRight);
  view_->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  view_->header()->setSectionResizeMode(HistoryModel::ColumnTitle, QHeaderView::Stretch);
  view_->header()->setSectionResizeMode(HistoryModel::ColumnDetails, QHeaderView::Stretch);

  layout()->addWidget(view_);

  initToolbar();

  connect(view_, &QWidget::customContextMenuRequested, this, &HistoryWidget::showContextMenu);
  connect(view_, &QTreeView::doubleClicked, this, &HistoryWidget::showMediaDialog);
}

void HistoryWidget::initToolbar() {
  const auto actionMore = new QAction(theme()->getIcon("more_horiz"), tr("More"), this);
  toolbar_->addAction(actionMore);

  auto* menu = new QMenu(this);
  menu->addAction(tr("Clear history..."), this, &HistoryWidget::clearHistory);
  menu->addAction(tr("Clear queue..."), this, &HistoryWidget::clearQueue);

  const auto moreButton = static_cast<QToolButton*>(toolbar_->widgetForAction(actionMore));
  moreButton->setPopupMode(QToolButton::InstantPopup);
  moreButton->setMenu(menu);
}

void HistoryWidget::showContextMenu() const {
  const auto index = view_->currentIndex();

  if (!index.isValid()) return;

  auto* menu = new QMenu(view_);
  menu->setAttribute(Qt::WA_DeleteOnClose);

  menu->addAction(theme()->getIcon("info"), tr("Details"), this,
                  [this, index]() { showMediaDialog(index); });
  menu->addAction(theme()->getIcon("delete"), tr("Remove..."), this,
                  [this, index]() { removeItem(index); });

  menu->popup(QCursor::pos());
}

void HistoryWidget::showMediaDialog(const QModelIndex& index) const {
  const auto historyItem = getHistoryItem(index);
  if (!historyItem) return;

  const auto item = anime::db()->item(historyItem->animeId);
  if (!item) return;

  MediaDialog::show(mainWindow(), MediaDialogPage::Details, *item);
}

void HistoryWidget::removeItem(const QModelIndex& index) const {
  const auto historyItem = getHistoryItem(index);
  if (!historyItem) return;

  const auto item = anime::db()->item(historyItem->animeId);
  const auto title = item ? QString::fromStdString(anime::preferredTitle(*item)) : tr("Unknown");

  if (confirm(nullptr, tr("Do you want to remove this entry from history?"),
              tr("%1 - Episode %2").arg(title).arg(historyItem->episode), tr("Remove"))) {
    anime::history()->remove(historyItem->id);
  }
}

void HistoryWidget::clearHistory() const {
  if (confirm(nullptr, tr("Do you want to clear your history?"),
              tr("All entries will be permanently removed."), tr("Clear"))) {
    anime::history()->clear();
  }
}

void HistoryWidget::clearQueue() const {
  const auto informativeText = tr("Any changes not yet synchronized with %1 will be discarded.")
                                   .arg(sync::serviceName(sync::currentServiceId()));

  if (confirm(nullptr, tr("Do you want to clear the sync queue?"), informativeText, tr("Clear"))) {
    sync::queue()->clear();
  }
}

const anime::HistoryItem* HistoryWidget::getHistoryItem(const QModelIndex& index) const {
  const int role = static_cast<int>(HistoryItemDataRole::HistoryItem);
  return index.data(role).value<const anime::HistoryItem*>();
}

}  // namespace gui
