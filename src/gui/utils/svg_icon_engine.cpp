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

#include "svg_icon_engine.hpp"

#include <QGuiApplication>
#include <QPainter>
#include <QPalette>
#include <utility>

#include "base/string.hpp"

namespace gui {

SvgIconEngine::SvgIconEngine(QString iconName) : iconName_(std::move(iconName)) {}

QString SvgIconEngine::key() const {
  return u"SvgIconEngine"_s;
}

QIconEngine* SvgIconEngine::clone() const {
  return new SvgIconEngine(iconName_);
}

QString SvgIconEngine::iconName() {
  return iconName_;
}

bool SvgIconEngine::isNull() {
  return false;
}

QPixmap SvgIconEngine::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) {
  return scaledPixmap(size, mode, state, 1.0);
}

QPixmap SvgIconEngine::scaledPixmap(const QSize& size, QIcon::Mode mode, QIcon::State state,
                                    qreal scale) {
  const quint64 cacheKey = (static_cast<quint64>(mode) << 32) | state;

  // Resolved on every call, so that icons follow palette changes (e.g. color scheme)
  const QColor color = [mode]() {
    const QPalette palette;
    switch (mode) {
      default:
      case QIcon::Normal:
        return palette.color(QPalette::Normal, QPalette::Text);
      case QIcon::Disabled:
        return palette.color(QPalette::Disabled, QPalette::Text);
      case QIcon::Active:
      // Item views select with a subtle tint rather than the highlight color (see main.qss), so
      // selected icons keep the normal text color.
      case QIcon::Selected:
        return palette.color(QPalette::Active, QPalette::Text);
    }
  }();

  if (cacheKey != cacheKey_ || color != color_ || size != size_ ||
      pixmap_.devicePixelRatio() != scale) {
    const QRect rect({0, 0}, size);

    cacheKey_ = cacheKey;
    color_ = color;
    size_ = size;
    pixmap_ = QPixmap(size * scale);
    pixmap_.fill(Qt::transparent);
    pixmap_.setDevicePixelRatio(scale);

    QPainter painter(&pixmap_);

    const auto icon = QIcon(u":/icons/%1.svg"_s.arg(iconName_));

    icon.paint(&painter, rect, Qt::AlignCenter);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(rect, color);
  }

  return pixmap_;
}

void SvgIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode,
                          QIcon::State state) {
  const qreal scale = painter->device()->devicePixelRatio();
  painter->drawPixmap(rect, scaledPixmap(rect.size(), mode, state, scale));
}

}  // namespace gui
