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

#include "header_view.hpp"

#include <QPainter>
#include <QPainterStateGuard>
#include <QStyle>
#include <QStyleOption>
#include <QTreeView>
#include <algorithm>

#include "gui/utils/theme.hpp"

namespace gui {

HeaderView::HeaderView(Qt::Orientation orientation, QWidget* parent)
    : QHeaderView(orientation, parent) {}

void HeaderView::install(QTreeView* view) {
  const auto oldHeader = view->header();
  const auto newHeader = new HeaderView(oldHeader->orientation(), view);
  newHeader->setSectionsMovable(oldHeader->sectionsMovable());
  newHeader->setStretchLastSection(oldHeader->stretchLastSection());
  newHeader->setDefaultAlignment(oldHeader->defaultAlignment());
  view->setHeader(newHeader);
}

void HeaderView::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const {
  if (!rect.isValid()) return;

  QStyleOptionHeaderV2 opt;
  initStyleOption(&opt);
  initStyleOptionForIndex(&opt, logicalIndex);
  opt.rect = rect;

  // Fusion draws the sort indicator over right-aligned labels. Other styles are left alone.
  if (!theme()->isFusion() || opt.sortIndicator == QStyleOptionHeader::None ||
      orientation() != Qt::Horizontal) {
    QHeaderView::paintSection(painter, rect, logicalIndex);
    return;
  }

  // Same as what QCommonStyle does for CE_Header, except that the label ends before the arrow.
  const QPainterStateGuard guard(painter);
  painter->setClipRect(rect);

  const QRect labelRect = style()->subElementRect(QStyle::SE_HeaderLabel, &opt, this);
  const QRect arrowRect = sortIndicatorRect(opt, labelRect);

  drawSection(painter, opt);
  drawLabel(painter, opt, labelRectBeforeArrow(opt, labelRect, arrowRect));
  drawSortIndicator(painter, opt, arrowRect);
}

QRect HeaderView::sortIndicatorRect(const QStyleOptionHeader& opt, const QRect& labelRect) const {
  QRect rect = style()->subElementRect(QStyle::SE_HeaderArrow, &opt, this);

  // Center the arrow on the label, which is where the text is vertically centered.
  rect.moveTop(labelRect.top() + (labelRect.height() - rect.height() + 1) / 2);

  // Move the arrow away from the section edge.
  rect.translate(opt.direction == Qt::RightToLeft ? kArrowEdgeOffset : -kArrowEdgeOffset, 0);

  return rect;
}

QRect HeaderView::labelRectBeforeArrow(const QStyleOptionHeader& opt, QRect labelRect,
                                       const QRect& arrowRect) const {
  if (opt.direction == Qt::RightToLeft) {
    labelRect.setLeft(std::max(labelRect.left(), arrowRect.right() + 1 + kArrowLabelGap));
  } else {
    labelRect.setRight(std::min(labelRect.right(), arrowRect.left() - 1 - kArrowLabelGap));
  }
  return labelRect;
}

void HeaderView::drawSection(QPainter* painter, const QStyleOptionHeaderV2& opt) const {
  style()->drawControl(QStyle::CE_HeaderSection, &opt, painter, this);
}

void HeaderView::drawLabel(QPainter* painter, QStyleOptionHeaderV2 opt, const QRect& rect) const {
  if (!rect.isValid()) return;
  opt.rect = rect;
  style()->drawControl(QStyle::CE_HeaderLabel, &opt, painter, this);
}

void HeaderView::drawSortIndicator(QPainter* painter, QStyleOptionHeaderV2 opt,
                                   const QRect& rect) const {
  opt.rect = rect;
  style()->drawPrimitive(QStyle::PE_IndicatorHeaderArrow, &opt, painter, this);
}

}  // namespace gui
