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

#include "svg_icon_engine.hpp"

#include <QGuiApplication>
#include <QPainter>
#include <QPalette>

namespace gui {

SvgIconEngine::SvgIconEngine(const QString& iconName) : m_iconName(iconName) {}

SvgIconEngine::~SvgIconEngine() {}

QString SvgIconEngine::key() const {
  return u"SvgIconEngine"_qs;
}

QIconEngine* SvgIconEngine::clone() const {
  return new SvgIconEngine(m_iconName);
}

QString SvgIconEngine::iconName() {
  return m_iconName;
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

  if (cacheKey != m_cacheKey || m_pixmap.size() != size || m_pixmap.devicePixelRatio() != scale) {
    const QRect rect({0, 0}, size);

    const QColor color = [mode]() {
      QPalette palette;
      switch (mode) {
        default:
        case QIcon::Normal:
          return palette.color(QPalette::Normal, QPalette::Text);
        case QIcon::Disabled:
          return palette.color(QPalette::Disabled, QPalette::Text);
        case QIcon::Active:
          return palette.color(QPalette::Active, QPalette::Text);
        case QIcon::Selected:
          return palette.color(QPalette::Active, QPalette::HighlightedText);
      }
    }();

    m_cacheKey = cacheKey;
    m_pixmap = QPixmap(size * scale);
    m_pixmap.fill(Qt::transparent);
    m_pixmap.setDevicePixelRatio(scale);

    QPainter painter(&m_pixmap);

    const auto icon = QIcon(u":/icons/%1.svg"_qs.arg(m_iconName));

    icon.paint(&painter, rect, Qt::AlignCenter);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(rect, color);
  }

  return m_pixmap;
}

void SvgIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode,
                          QIcon::State state) {
  const qreal scale = painter->device()->devicePixelRatio();
  painter->drawPixmap(rect, scaledPixmap(rect.size(), mode, state, scale));
}

}  // namespace gui
