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

#pragma once

#include <QHeaderView>
#include <QStyleOption>

class QTreeView;

namespace gui {

class HeaderView final : public QHeaderView {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(HeaderView)

public:
  HeaderView(Qt::Orientation orientation, QWidget* parent = nullptr);

  static void install(QTreeView* view);

protected:
  void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override;

private:
  static constexpr int kArrowEdgeOffset = 4;
  static constexpr int kArrowLabelGap = 6;

  QRect sortIndicatorRect(const QStyleOptionHeader& opt, const QRect& labelRect) const;
  QRect labelRectBeforeArrow(const QStyleOptionHeader& opt, QRect labelRect,
                             const QRect& arrowRect) const;

  void drawSection(QPainter* painter, const QStyleOptionHeaderV2& opt) const;
  void drawLabel(QPainter* painter, QStyleOptionHeaderV2 opt, const QRect& rect) const;
  void drawSortIndicator(QPainter* painter, QStyleOptionHeaderV2 opt, const QRect& rect) const;
};

}  // namespace gui
