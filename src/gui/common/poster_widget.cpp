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

#include "poster_widget.hpp"

#include <QPainter>
#include <QPainterPath>

#include "gui/common/spinner_widget.hpp"
#include "gui/utils/theme.hpp"

namespace gui {

constexpr int kSpinnerSize = 20;

PosterWidget::PosterWidget(QWidget* parent) : QWidget(parent) {
  m_spinner = new SpinnerWidget(this, kSpinnerSize);
}

void PosterWidget::setCornerRadius(const qreal radius) {
  if (radius == m_cornerRadius) return;

  m_cornerRadius = radius;

  update();
}

void PosterWidget::setPixmap(const QPixmap& pixmap) {
  if (pixmap.cacheKey() == m_pixmap.cacheKey()) return;

  m_pixmap = pixmap;
  m_scaledPixmap = QPixmap{};

  update();
}

void PosterWidget::setLoading(const bool loading) {
  if (loading == m_loading) return;

  m_loading = loading;

  if (m_loading) {
    m_spinner->start();
  } else {
    m_spinner->stop();
  }
}

void PosterWidget::paintEvent(QPaintEvent*) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  if (m_cornerRadius > 0) {
    QPainterPath path;
    path.addRoundedRect(rect(), m_cornerRadius, m_cornerRadius);
    painter.setClipPath(path);
  }

  painter.fillRect(rect(), theme.isDark() ? palette().dark() : palette().mid());

  if (m_pixmap.isNull()) return;

  const QSize targetSize = size() * devicePixelRatioF();
  if (m_scaledPixmap.isNull() || m_scaledSize != targetSize) {
    m_scaledPixmap =
        m_pixmap.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    m_scaledSize = targetSize;
  }

  const QRectF sourceRect{
      (m_scaledPixmap.width() - targetSize.width()) / 2.0,
      (m_scaledPixmap.height() - targetSize.height()) / 2.0,
      static_cast<qreal>(targetSize.width()),
      static_cast<qreal>(targetSize.height()),
  };
  painter.drawPixmap(rect(), m_scaledPixmap, sourceRect);
}

void PosterWidget::resizeEvent(QResizeEvent*) {
  m_spinner->move((width() - m_spinner->width()) / 2, (height() - m_spinner->height()) / 2);
}

}  // namespace gui
