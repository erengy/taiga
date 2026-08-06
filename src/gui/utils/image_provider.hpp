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

#include <QMap>
#include <QObject>
#include <QPixmap>
#include <QRestAccessManager>
#include <QString>

namespace gui {

class ImageProvider final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(ImageProvider)

public:
  ImageProvider();

  void fetchPoster(const int id, const bool revalidate = false);
  QPixmap loadPoster(const int id);
  void reloadPoster(const int id);

signals:
  void posterChanged(const int id);

private:
  QString fileName(const int id) const;
  bool isStale(const int id) const;

  QRestAccessManager m_manager;
  QMap<int, QPixmap> m_pixmaps;
};

inline ImageProvider imageProvider;

}  // namespace gui
