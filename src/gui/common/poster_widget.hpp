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

#include <QPixmap>
#include <QSize>
#include <QWidget>

namespace gui {

class SpinnerWidget;

class PosterWidget final : public QWidget {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(PosterWidget)

public:
  explicit PosterWidget(QWidget* parent = nullptr);
  ~PosterWidget() = default;

  void setCornerRadius(const qreal radius);
  void setPixmap(const QPixmap& pixmap);
  void setLoading(const bool loading);

protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

private:
  qreal m_cornerRadius = 0;
  QPixmap m_pixmap;
  QPixmap m_scaledPixmap;
  QSize m_scaledSize;
  SpinnerWidget* m_spinner = nullptr;
  bool m_loading = false;
};

}  // namespace gui
