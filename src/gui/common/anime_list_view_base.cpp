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

#include "anime_list_view_base.hpp"

#include <QDesktopServices>
#include <QLineEdit>
#include <QListView>
#include <QTreeView>
#include <QUrl>

#include "gui/main/main_window.hpp"
#include "gui/main/navigation_item_delegate.hpp"
#include "gui/main/navigation_widget.hpp"
#include "gui/main/now_playing_widget.hpp"
#include "gui/main/status_bar_controller.hpp"
#include "gui/media/media_dialog.hpp"
#include "gui/media/media_menu.hpp"
#include "gui/models/anime_list_model.hpp"
#include "gui/models/anime_list_proxy_model.hpp"
#include "gui/utils/format.hpp"
#include "media/anime.hpp"
#include "media/anime_list.hpp"
#include "media/anime_utils.hpp"
#include "sync/service.hpp"
#include "track/play.hpp"

namespace gui {

ListViewBase::ListViewBase(QWidget* parent, QAbstractItemView* view, AnimeListModel* model,
                           AnimeListProxyModel* proxyModel, AnimeListContext context)
    : QObject(parent), model_(model), proxyModel_(proxyModel), view_(view), context_(context) {
  view_->setContextMenuPolicy(Qt::CustomContextMenu);
  view_->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

  proxyModel_->setSourceModel(model_);
  view_->setModel(proxyModel_);

  connect(mainWindow()->searchBox(), &QLineEdit::textChanged, this, &ListViewBase::filterByText);

  connect(view_, &QAbstractItemView::doubleClicked, this, &ListViewBase::showMediaDialog);

  connect(view_, &QWidget::customContextMenuRequested, this, &ListViewBase::showMediaMenu);

  connect(view_->selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &ListViewBase::updateSelectionStatus);
}

void ListViewBase::filterByText(const QString& text) {
  proxyModel_->setTextFilter(text);
}

void ListViewBase::openAnimePage(const QModelIndex& index) {
  const auto mappedIndex = proxyModel_->mapToSource(index);
  const auto anime = model_->getAnime(mappedIndex);
  if (!anime) return;
  QDesktopServices::openUrl(QUrl{sync::animePageUrl(anime->id)});
}

void ListViewBase::playNextEpisode(const QModelIndex& index) {
  const auto mappedIndex = proxyModel_->mapToSource(index);
  const auto item = model_->getAnime(mappedIndex);
  if (!item) return;

  const auto number = track::nextEpisodeNumber(item->id);

  if (!number) {
    mainWindow()->statusBarController()->clearMessage(StatusBarController::Source::Playback);
    return;
  }

  if (track::playEpisode(item->id, *number)) {
    mainWindow()->statusBarController()->clearMessage(StatusBarController::Source::Playback);
    return;
  }

  mainWindow()->statusBarController()->showMessage({
      .source = StatusBarController::Source::Playback,
      .text = tr("Could not find episode #%1 (%2).").arg(*number).arg(anime::preferredTitle(*item)),
      .spin = false,
  });
}

void ListViewBase::showMediaDialog(const QModelIndex& index) {
  const auto mappedIndex = proxyModel_->mapToSource(index);
  const auto anime = model_->getAnime(mappedIndex);
  if (!anime) return;
  MediaDialog::show(mainWindow(), MediaDialogPage::Details, *anime);
}

void ListViewBase::showMediaMenu() {
  const auto indexes = selectedIndexes();
  if (indexes.isEmpty()) return;

  QList<Anime> items;
  QMap<int, ListEntry> entries;

  for (auto selectedIndex : indexes) {
    const auto index = proxyModel_->mapToSource(selectedIndex);
    if (const auto item = model_->getAnime(index)) {
      items.push_back(*item);
      if (const auto entry = model_->getListEntry(index)) {
        entries[item->id] = *entry;
      }
    }
  }

  auto* menu = new MediaMenu(view_, items, entries, view_->selectionModel(), context_);
  menu->popup();
}

void ListViewBase::updateSelectionStatus(const QItemSelection&, const QItemSelection&) {
  const auto nSelected = selectedIndexes().size();

  if (!nSelected) {
    mainWindow()->statusBarController()->clearMessage(StatusBarController::Source::Selection);
    return;
  }

  int nEpisodes = 0;
  int nScore = 0;
  double totalScore = 0.0;
  double averageScore = 0.0;

  for (const auto index : selectedIndexes()) {
    const auto anime = model_->getAnime(proxyModel_->mapToSource(index));
    if (!anime) continue;
    if (anime->episodeCount > 0) nEpisodes += anime->episodeCount;
    if (anime->score) {
      ++nScore;
      totalScore += anime->score;
    }
  }

  if (nScore) averageScore = totalScore / nScore;

  const QStringList parts{
      tr("%n item(s) selected", nullptr, nSelected),
      tr("%n episode(s)", nullptr, nEpisodes),
      tr("%1 average").arg(formatScore(averageScore)),
  };

  mainWindow()->statusBarController()->showMessage({
      .source = StatusBarController::Source::Selection,
      .text = parts.join(" · "),
      .spin = false,
  });
}

QModelIndexList ListViewBase::selectedIndexes() {
  const auto model = view_->selectionModel();
  return !model->selectedRows().empty() ? model->selectedRows() : model->selectedIndexes();
}

}  // namespace gui
