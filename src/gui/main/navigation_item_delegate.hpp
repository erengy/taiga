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

#include <QStyledItemDelegate>

namespace gui {

enum class NavigationItemDataRole {
  PageIndex = Qt::UserRole,
  HasChildren,
  IsChild,
  IsLastChild,
  IsSeparator,
  Counter,
};

class NavigationItemDelegate final : public QStyledItemDelegate {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(NavigationItemDelegate)

public:
  NavigationItemDelegate(QObject* parent);
  ~NavigationItemDelegate() = default;

  void paint(QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const override;

private:
  void paintBranch(QPainter*, QRect, bool) const;
  void paintCounter(QPainter*, QRect, const int) const;
  void paintSeparator(QPainter*, const QRect&) const;
};

}  // namespace gui
