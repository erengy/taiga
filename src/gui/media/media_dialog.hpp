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

#include <QDialog>
#include <QPixmap>

#include "media/anime.hpp"

class QResizeEvent;
class QShowEvent;

namespace Ui {
class MediaDialog;
}

namespace gui {

class MediaDialog final : public QDialog {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(MediaDialog)

public:
  MediaDialog(QWidget* parent);
  ~MediaDialog() = default;

  void resizeEvent(QResizeEvent* event) override;
  void showEvent(QShowEvent* event) override;

  static void show(QWidget* parent, const Anime& anime_item);

public slots:
  void setAnime(const Anime& anime_item);

private:
  void initDetails() const;
  void loadPosterImage();
  void resizePosterImage();

  Ui::MediaDialog* ui_ = nullptr;

  Anime m_anime;
  QPixmap m_pixmap;
};

}  // namespace gui
