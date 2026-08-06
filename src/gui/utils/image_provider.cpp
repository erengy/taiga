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

#include "image_provider.hpp"

#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QNetworkRequest>
#include <QRestReply>

#include "base/string.hpp"
#include "media/anime_db.hpp"
#include "taiga/network.hpp"
#include "taiga/path.hpp"

namespace gui {

ImageProvider::ImageProvider() : m_manager(taiga::network(), this) {}

void ImageProvider::fetchPoster(const int id) {
  const auto item = anime::db.item(id);

  if (!item || item->image_url.empty()) return;

  const auto url = QString::fromStdString(item->image_url);

  m_manager.get(QNetworkRequest{url}, this, [this, id](QRestReply& reply) {
    QFile file{fileName(id)};
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(reply.readBody());
    reloadPoster(id);
  });
}

const QPixmap* ImageProvider::loadPoster(const int id) {
  if (const auto it = m_pixmaps.find(id); it != m_pixmaps.end()) {
    return &it.value();
  }

  QImageReader reader(fileName(id));
  const QImage image = reader.read();

  m_pixmaps[id] = !image.isNull() ? QPixmap::fromImage(image) : QPixmap{};

  if (image.isNull()) fetchPoster(id);

  return &m_pixmaps[id];
}

void ImageProvider::reloadPoster(const int id) {
  m_pixmaps.remove(id);
  loadPoster(id);
  emit posterChanged(id);
}

QString ImageProvider::fileName(const int id) const {
  const auto path = QString::fromStdString(taiga::get_data_path());
  return u"%1/v1/db/image/%2.jpg"_s.arg(path).arg(id);  // @TODO: Support other formats (#1191)
}

}  // namespace gui
