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

#include "navigation_item_delegate.hpp"

#include <QPainter>
#include <QPainterPath>

#include "gui/utils/painter_state_saver.hpp"
#include "gui/utils/theme.hpp"

namespace gui {

static const auto lineColorLight = QColor{0, 0, 0, 20};
static const auto lineColorDark = QColor{255, 255, 255, 20};

NavigationItemDelegate::NavigationItemDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void NavigationItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const {
  // Separator
  if (index.data(static_cast<int>(NavigationItemDataRole::IsSeparator)).toBool()) {
    paintSeparator(painter, option.rect);
    return;
  }

  QStyledItemDelegate::paint(painter, option, index);

  // Branch
  if (index.data(static_cast<int>(NavigationItemDataRole::IsChild)).toBool()) {
    const bool isLastChild =
        index.data(static_cast<int>(NavigationItemDataRole::IsLastChild)).toBool();
    paintBranch(painter, option.rect, isLastChild);
  }

  // Counter
  if (const int count = index.data(static_cast<int>(NavigationItemDataRole::Counter)).toInt()) {
    paintCounter(painter, option.rect, count);
  }
}

void NavigationItemDelegate::paintBranch(QPainter* painter, QRect rect, bool isLastChild) const {
  const PainterStateSaver painterStateSaver(painter);

  rect.setLeft(4);
  rect.setWidth(16);

  const QPoint center = rect.center();

  painter->setPen([painter]() {
    auto pen = painter->pen();
    pen.setColor(theme.isDark() ? lineColorDark : lineColorLight);
    return pen;
  }());

  if (isLastChild) {
    painter->drawLine({center.x(), rect.top()}, center);
  } else {
    painter->drawLine(center.x(), rect.top(), center.x(), rect.bottom());
  }

  painter->drawLine(center.x() + painter->pen().width(), center.y(), rect.right(), center.y());
}

void NavigationItemDelegate::paintCounter(QPainter* painter, QRect rect, const int count) const {
  const PainterStateSaver painterStateSaver(painter);

  painter->setFont([painter]() {
    auto font = painter->font();
    font.setPointSize(8);
    font.setWeight(QFont::Weight::DemiBold);
    return font;
  }());

  const QString text = u"%1"_qs.arg(count);
  const QFontMetrics metrics(painter->font());
  const QRect boundingRect = metrics.boundingRect(text);

  rect.setRight(rect.right() - 8);
  rect.setLeft(rect.right() - boundingRect.width());
  rect.setTop(rect.top() + ((rect.height() - boundingRect.height()) / 2));
  rect.setHeight(boundingRect.height());
  rect.adjust(-4, -1, 4, 1);
  if (rect.width() < rect.height()) {
    rect.setLeft(rect.right() - rect.height());
  }

  QPainterPath path;
  path.addRoundedRect(rect, 8, 8);
  painter->setClipPath(path);

  painter->fillRect(rect, theme.isDark() ? lineColorDark : lineColorLight);

  painter->setPen(QColor(theme.isDark() ? 0x888888 : 0x666666));
  painter->drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine, text);
}

void NavigationItemDelegate::paintSeparator(QPainter* painter, const QRect& rect) const {
  const PainterStateSaver painterStateSaver(painter);

  const int y = rect.center().y();

  painter->setPen(theme.isDark() ? lineColorDark : lineColorLight);
  painter->drawLine(rect.left() + 4, y, rect.right() - 4, y);
};

}  // namespace gui
