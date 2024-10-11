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

#include "anime_list_proxy_model.hpp"

#include <ranges>

#include "gui/models/anime_list_model.hpp"
#include "media/anime.hpp"
#include "media/anime_season.hpp"

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
  setFilterKeyColumn(AnimeListModel::COLUMN_TITLE);
  setFilterRole(Qt::DisplayRole);

  setSortCaseSensitivity(Qt::CaseInsensitive);
  setSortRole(Qt::UserRole);
}

void AnimeListProxyModel::setYearFilter(std::optional<int> year) {
  m_filter.year = year;
  invalidateRowsFilter();
}

void AnimeListProxyModel::setSeasonFilter(std::optional<int> season) {
  m_filter.season = season;
  invalidateRowsFilter();
}

void AnimeListProxyModel::setTypeFilter(std::optional<int> type) {
  m_filter.type = type;
  invalidateRowsFilter();
}

void AnimeListProxyModel::setStatusFilter(std::optional<int> status) {
  m_filter.status = status;
  invalidateRowsFilter();
}

void AnimeListProxyModel::setListStatusFilter(std::optional<int> status) {
  m_filter.listStatus = status;
  invalidateRowsFilter();
}

void AnimeListProxyModel::setTextFilter(const QString& text) {
  m_filter.text = text;
  invalidateRowsFilter();
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

  static const auto list_contains = [](const std::vector<std::string>& list,
                                       const QStringView view) {
    return std::ranges::any_of(list,
                               [view](const std::string& str) { return contains(str, view); });
  };

  // Year
  if (m_filter.year) {
    if (anime->start_date.year() != *m_filter.year) return false;
  }

  // Season
  if (m_filter.season) {
    const anime::Season season{anime->start_date};
    if (static_cast<int>(season.name) != *m_filter.season) return false;
  }

  // Type
  if (m_filter.type) {
    if (static_cast<int>(anime->type) != *m_filter.type) return false;
  }

  // Status
  if (m_filter.status) {
    if (static_cast<int>(anime->status) != *m_filter.status) return false;
  }

  // List status
  if (m_filter.listStatus) {
    const auto status = static_cast<int>(entry ? entry->status : anime::list::Status::NotInList);
    if (status != *m_filter.listStatus) return false;
  }

  // Titles
  if (!m_filter.text.isEmpty()) {
    if (!contains(anime->titles.romaji, m_filter.text) &&
        !contains(anime->titles.english, m_filter.text) &&
        !contains(anime->titles.japanese, m_filter.text) &&
        !list_contains(anime->titles.synonyms, m_filter.text)) {
      return false;
    }
  }

  return true;
}

bool AnimeListProxyModel::lessThan(const QModelIndex& lhs, const QModelIndex& rhs) const {
  const auto lhs_anime = getAnime(lhs);
  const auto rhs_anime = getAnime(rhs);
  if (!lhs_anime || !rhs_anime) return false;

  const auto lhs_entry = getListEntry(lhs);
  const auto rhs_entry = getListEntry(rhs);

  switch (lhs.column()) {
    case AnimeListModel::COLUMN_TITLE:
      return lhs_anime->titles.romaji < rhs_anime->titles.romaji;

    case AnimeListModel::COLUMN_DURATION:
      return lhs_anime->episode_length < rhs_anime->episode_length;

    case AnimeListModel::COLUMN_AVERAGE:
      return lhs_anime->score < rhs_anime->score;

    case AnimeListModel::COLUMN_TYPE:
      return lhs_anime->type < rhs_anime->type;

    case AnimeListModel::COLUMN_PROGRESS:
      return (lhs_entry ? lhs_entry->watched_episodes : 0) <
             (rhs_entry ? rhs_entry->watched_episodes : 0);

    case AnimeListModel::COLUMN_REWATCHES:
      return (lhs_entry ? lhs_entry->rewatched_times : 0) <
             (rhs_entry ? rhs_entry->rewatched_times : 0);

    case AnimeListModel::COLUMN_SCORE:
      return (lhs_entry ? lhs_entry->score : 0) < (rhs_entry ? rhs_entry->score : 0);

    case AnimeListModel::COLUMN_SEASON:
      return lhs_anime->start_date < rhs_anime->start_date;

    case AnimeListModel::COLUMN_STARTED:
      return (lhs_entry ? lhs_entry->date_start : FuzzyDate{}) <
             (rhs_entry ? rhs_entry->date_start : FuzzyDate{});

    case AnimeListModel::COLUMN_COMPLETED:
      return (lhs_entry ? lhs_entry->date_finish : FuzzyDate{}) <
             (rhs_entry ? rhs_entry->date_finish : FuzzyDate{});

    case AnimeListModel::COLUMN_LAST_UPDATED:
      return (lhs_entry ? lhs_entry->last_updated : 0) < (rhs_entry ? rhs_entry->last_updated : 0);

    case AnimeListModel::COLUMN_NOTES:
      return (lhs_entry ? lhs_entry->notes : "") < (rhs_entry ? rhs_entry->notes : "");
  }

  return false;
}

}  // namespace gui
