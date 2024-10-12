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

struct AnimeListStatusFilter {
  std::optional<int> status;
  bool anyStatus = false;
};

struct AnimeListProxyModelFilter {
  std::optional<int> year;
  std::optional<int> season;
  std::optional<int> type;
  std::optional<int> status;
  AnimeListStatusFilter listStatus;
  QString text;
};

class AnimeListProxyModel final : public QSortFilterProxyModel {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(AnimeListProxyModel)

public:
  AnimeListProxyModel(QObject* parent);
  ~AnimeListProxyModel() = default;

  const AnimeListProxyModelFilter& filters() const {
    return m_filter;
  }

  void setYearFilter(std::optional<int> year);
  void setSeasonFilter(std::optional<int> season);
  void setTypeFilter(std::optional<int> type);
  void setStatusFilter(std::optional<int> status);
  void setListStatusFilter(AnimeListStatusFilter filter);
  void setTextFilter(const QString& text);

protected:
  bool filterAcceptsRow(int row, const QModelIndex& parent) const override;
  bool lessThan(const QModelIndex& lhs, const QModelIndex& rhs) const override;

private:
  AnimeListProxyModelFilter m_filter;
};

}  // namespace gui
