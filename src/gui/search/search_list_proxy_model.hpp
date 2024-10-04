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

#include <QSortFilterProxyModel>
#include <optional>

namespace gui {

struct SearchListProxyModelFilter {
  std::optional<int> year;
  std::optional<int> season;
  std::optional<int> type;
  std::optional<int> status;
};

class SearchListProxyModel final : public QSortFilterProxyModel {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(SearchListProxyModel)

public:
  SearchListProxyModel(QObject* parent);
  ~SearchListProxyModel() = default;

  void setYearFilter(int year);
  void setSeasonFilter(int season);
  void setTypeFilter(int type);
  void setStatusFilter(int status);

protected:
  bool filterAcceptsRow(int row, const QModelIndex& parent) const override;

private:
  SearchListProxyModelFilter m_filter;
};

}  // namespace gui
