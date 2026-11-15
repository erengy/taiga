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

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include "gui/common/spinner_widget.hpp"
#include "gui/utils/theme.hpp"

namespace gui {

constexpr int kDefaultSpinnerSize = 24;

PosterWidget::PosterWidget(QWidget* parent) : QWidget(parent) {
  spinner_ = new SpinnerWidget(this, kDefaultSpinnerSize);
}

const QPixmap& PosterWidget::pixmap() const {
  return pixmap_;
}

void PosterWidget::setCornerRadius(const qreal radius) {
  if (radius == cornerRadius_) return;

  cornerRadius_ = radius;

  update();
}

void PosterWidget::setPixmap(const QPixmap& pixmap) {
  if (pixmap.cacheKey() == pixmap_.cacheKey()) return;

  pixmap_ = pixmap;
  scaledPixmap_ = QPixmap{};

  update();
}

void PosterWidget::setSpinnerSize(const int size) {
  spinner_->setSize(size);
  positionSpinner();
}

void PosterWidget::setLoading(const bool loading) {
  if (loading == loading_) return;

  loading_ = loading;

  if (loading_) {
    spinner_->start();
  } else {
    spinner_->stop();
  }
}

void PosterWidget::mousePressEvent(QMouseEvent* event) {
  emit clicked(event->button());

  QWidget::mousePressEvent(event);
}

void PosterWidget::paintEvent(QPaintEvent*) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  if (cornerRadius_ > 0) {
    QPainterPath path;
    path.addRoundedRect(rect(), cornerRadius_, cornerRadius_);
    painter.setClipPath(path);
  }

  painter.fillRect(rect(), theme()->isDark() ? palette().dark() : palette().mid());

  if (pixmap_.isNull()) return;

  const QSize targetSize = size() * devicePixelRatioF();
  if (scaledPixmap_.isNull() || scaledSize_ != targetSize) {
    scaledPixmap_ =
        pixmap_.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    scaledSize_ = targetSize;
  }

  const QRectF sourceRect{
      (scaledPixmap_.width() - targetSize.width()) / 2.0,
      (scaledPixmap_.height() - targetSize.height()) / 2.0,
      static_cast<qreal>(targetSize.width()),
      static_cast<qreal>(targetSize.height()),
  };
  painter.drawPixmap(rect(), scaledPixmap_, sourceRect);
}

void PosterWidget::resizeEvent(QResizeEvent*) {
  positionSpinner();
}

void PosterWidget::positionSpinner() {
  spinner_->move((width() - spinner_->width()) / 2, (height() - spinner_->height()) / 2);
}

}  // namespace gui
