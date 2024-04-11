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

#include <QIconEngine>

namespace gui {

class SvgIconEngine : public QIconEngine {
public:
  SvgIconEngine(const QString& iconName);
  ~SvgIconEngine();

  QString key() const override;
  QIconEngine* clone() const override;
  QString iconName() override;
  bool isNull() override;

  QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;
  QPixmap scaledPixmap(const QSize& size, QIcon::Mode mode, QIcon::State state,
                       qreal scale) override;
  void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;

private:
  const QString m_iconName;
  QPixmap m_pixmap;
  quint64 m_cacheKey{};
};

}  // namespace gui
