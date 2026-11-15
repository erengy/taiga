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

#include "anime_list_proxy_model.hpp"

#include <ranges>

#include "base/string.hpp"
#include "gui/models/anime_list_model.hpp"
#include "media/anime.hpp"
#include "media/anime_list.hpp"
#include "media/anime_list_utils.hpp"
#include "media/anime_season.hpp"
#include "media/anime_utils.hpp"

namespace {

const Anime* getAnime(const QModelIndex& index) {
  const int role = static_cast<int>(gui::AnimeListItemDataRole::Anime);
  return index.data(role).value<const Anime*>();
}

const ListEntry* getListEntry(const QModelIndex& index) {
  const int role = static_cast<int>(gui::AnimeListItemDataRole::ListEntry);
  return index.data(role).value<const ListEntry*>();
}

}  // namespace

namespace gui {

AnimeListProxyModel::AnimeListProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {
  setFilterKeyColumn(AnimeListModel::ColumnTitle);
  setFilterRole(Qt::DisplayRole);

  setSortCaseSensitivity(Qt::CaseInsensitive);
  setSortRole(Qt::UserRole);
}

const AnimeListProxyModelFilter& AnimeListProxyModel::filters() const {
  return filter_;
}

void AnimeListProxyModel::setFilters(const AnimeListProxyModelFilter& filters) {
  beginFilterChange();
  filter_ = filters;
  endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

void AnimeListProxyModel::setYearFilter(std::optional<int> year) {
  beginFilterChange();
  filter_.year = year;
  endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

void AnimeListProxyModel::setSeasonFilter(std::optional<int> season) {
  beginFilterChange();
  filter_.season = season;
  endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

void AnimeListProxyModel::setTypeFilter(std::optional<int> type) {
  beginFilterChange();
  filter_.type = type;
  endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

void AnimeListProxyModel::setStatusFilter(std::optional<int> status) {
  beginFilterChange();
  filter_.status = status;
  endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

void AnimeListProxyModel::setListStatusFilter(AnimeListStatusFilter filter) {
  beginFilterChange();
  filter_.listStatus = filter;
  endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

void AnimeListProxyModel::setTextFilter(const QString& text) {
  beginFilterChange();
  filter_.text = text;
  endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

void AnimeListProxyModel::setNsfwHidden(bool hidden) {
  beginFilterChange();
  nsfwHidden_ = hidden;
  endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

bool AnimeListProxyModel::filterAcceptsRow(int row, const QModelIndex& parent) const {
  const auto model = static_cast<AnimeListModel*>(sourceModel());
  if (!model) return false;
  const auto index = model->index(row, 0, parent);
  const auto anime = getAnime(index);
  if (!anime) return false;
  const auto entry = getListEntry(index);

  static const auto contains = [](const std::string& str, const QStringView view) {
    return QString::fromStdString(str).contains(view, Qt::CaseInsensitive);
  };

  static const auto listContains = [](const std::vector<std::string>& list,
                                      const QStringView view) {
    return std::ranges::any_of(list,
                               [view](const std::string& str) { return contains(str, view); });
  };

  // Age rating
  if (nsfwHidden_ && anime::isNsfw(*anime)) return false;

  // Year
  if (filter_.year) {
    if (anime->dateStarted.year() != *filter_.year) return false;
  }

  // Season
  if (filter_.season) {
    const anime::Season season{anime->dateStarted};
    if (static_cast<int>(season.name) != *filter_.season) return false;
  }

  // Type
  if (filter_.type) {
    if (static_cast<int>(anime->type) != *filter_.type) return false;
  }

  // Status
  if (filter_.status) {
    if (static_cast<int>(anime->status) != *filter_.status) return false;
  }

  // List status
  if (filter_.listStatus.status) {
    auto status = anime::list::Status::NotInList;
    if (entry && !entry->pendingDelete) status = entry->status;
    if (filter_.listStatus.anyStatus) {
      if (status == anime::list::Status::NotInList) return false;
    } else {
      if (static_cast<int>(status) != *filter_.listStatus.status) return false;
    }
  }

  // Titles
  if (!filter_.text.isEmpty()) {
    if (!contains(anime->titles.romaji, filter_.text) &&
        !contains(anime->titles.english, filter_.text) &&
        !contains(anime->titles.japanese, filter_.text) &&
        !listContains(anime->titles.synonyms, filter_.text)) {
      return false;
    }
  }

  return true;
}

int AnimeListProxyModel::secondarySortColumn() const {
  return secondarySortColumn_;
}

Qt::SortOrder AnimeListProxyModel::secondarySortOrder() const {
  return secondarySortOrder_;
}

void AnimeListProxyModel::setSecondarySort(int column, Qt::SortOrder order) {
  secondarySortColumn_ = column;
  secondarySortOrder_ = order;
}

void AnimeListProxyModel::sort(int column, Qt::SortOrder order) {
  // The previous primary sort becomes the secondary sort, which orders items
  // that are equal in the primary comparison.
  if (sortColumn() > -1 && sortColumn() != column) {
    setSecondarySort(sortColumn(), sortOrder());
  }

  QSortFilterProxyModel::sort(column, order);
}

bool AnimeListProxyModel::lessThan(const QModelIndex& lhs, const QModelIndex& rhs) const {
  const int primaryColumn = lhs.column();

  if (const auto result = compare(lhs, rhs, primaryColumn); result != 0) {
    return result < 0;
  }

  // Qt swaps arguments for descending order, which also flips tie-breakers.
  const auto compareInOrder = [&](int column, Qt::SortOrder order) {
    return order == sortOrder() ? compare(lhs, rhs, column) : compare(rhs, lhs, column);
  };

  if (secondarySortColumn_ > -1 && secondarySortColumn_ != primaryColumn) {
    const auto result = compareInOrder(secondarySortColumn_, secondarySortOrder_);
    if (result != 0) return result < 0;
  }

  // Fall back to title, so that the order doesn't depend on the source model.
  return compareInOrder(AnimeListModel::ColumnTitle, Qt::AscendingOrder) < 0;
}

std::partial_ordering AnimeListProxyModel::compare(const QModelIndex& lhs, const QModelIndex& rhs,
                                                   int column) const {
  const auto lhsAnime = getAnime(lhs);
  const auto rhsAnime = getAnime(rhs);
  if (!lhsAnime || !rhsAnime) return std::partial_ordering::equivalent;

  const auto lhsEntry = getListEntry(lhs);
  const auto rhsEntry = getListEntry(rhs);

  switch (column) {
    case AnimeListModel::ColumnTitle:
      return compareStrings(anime::preferredTitle(*lhsAnime), anime::preferredTitle(*rhsAnime),
                            Qt::CaseInsensitive) <=> 0;

    case AnimeListModel::ColumnDuration:
      return lhsAnime->episodeLength <=> rhsAnime->episodeLength;

    case AnimeListModel::ColumnAverage:
      return lhsAnime->score <=> rhsAnime->score;

    case AnimeListModel::ColumnType:
      return lhsAnime->type <=> rhsAnime->type;

    case AnimeListModel::ColumnProgress: {
      const auto lhsRatio = anime::list::getProgressRatio(lhsAnime, lhsEntry);
      const auto rhsRatio = anime::list::getProgressRatio(rhsAnime, rhsEntry);
      if (const auto result = lhsRatio <=> rhsRatio; result != 0) return result;
      return anime::estimateEpisodeCount(*lhsAnime, 0) <=>
             anime::estimateEpisodeCount(*rhsAnime, 0);
    }

    case AnimeListModel::ColumnRewatches:
      return (lhsEntry ? lhsEntry->rewatchedTimes : 0) <=>
             (rhsEntry ? rhsEntry->rewatchedTimes : 0);

    case AnimeListModel::ColumnScore:
      return (lhsEntry ? lhsEntry->score : 0) <=> (rhsEntry ? rhsEntry->score : 0);

    case AnimeListModel::ColumnSeason:
      return lhsAnime->dateStarted <=> rhsAnime->dateStarted;

    case AnimeListModel::ColumnStarted:
      return (lhsEntry ? lhsEntry->dateStarted : FuzzyDate{}) <=>
             (rhsEntry ? rhsEntry->dateStarted : FuzzyDate{});

    case AnimeListModel::ColumnCompleted:
      return (lhsEntry ? lhsEntry->dateCompleted : FuzzyDate{}) <=>
             (rhsEntry ? rhsEntry->dateCompleted : FuzzyDate{});

    case AnimeListModel::ColumnLastUpdated:
      return (lhsEntry ? lhsEntry->lastUpdated : 0) <=> (rhsEntry ? rhsEntry->lastUpdated : 0);

    case AnimeListModel::ColumnNotes:
      return (lhsEntry ? lhsEntry->notes : "") <=> (rhsEntry ? rhsEntry->notes : "");
  }

  return std::partial_ordering::equivalent;
}

}  // namespace gui
