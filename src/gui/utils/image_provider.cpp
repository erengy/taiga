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

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QNetworkRequest>
#include <QRestReply>
#include <QUrl>

#include "base/string.hpp"
#include "media/anime_db.hpp"
#include "sync/service.hpp"
#include "taiga/network.hpp"
#include "taiga/path.hpp"

namespace gui {

ImageProvider::ImageProvider() : m_manager(taiga::network(), this) {}

void ImageProvider::fetchPoster(const int id) {
  const auto item = anime::db.item(id);

  if (!item || item->image_url.empty()) return;

  const auto url = QString::fromStdString(item->image_url);

  m_manager.get(QNetworkRequest{url}, this, [this, id](QRestReply& reply) {
    if (!reply.isHttpStatusSuccess() || reply.hasError()) {
      if (reply.httpStatus() == 404) {
        if (const auto item = anime::db.item(id)) {
          auto updatedItem = *item;
          updatedItem.image_url.clear();
          anime::db.updateItem(updatedItem);
        }
      }
      return;
    }

    QFile file{fileName(id)};
    QDir().mkpath(QFileInfo(file).path());
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(reply.readBody());
    reloadPoster(id);
  });
}

QPixmap ImageProvider::loadPoster(const int id) {
  if (const auto it = m_pixmaps.find(id); it != m_pixmaps.end()) {
    return it.value();
  }

  QImageReader reader(fileName(id));
  const QImage image = reader.read();

  const auto pixmap = !image.isNull() ? QPixmap::fromImage(image) : QPixmap{};
  m_pixmaps[id] = pixmap;

  if (image.isNull()) fetchPoster(id);

  return pixmap;
}

void ImageProvider::reloadPoster(const int id) {
  m_pixmaps.remove(id);
  loadPoster(id);
  emit posterChanged(id);
}

QString ImageProvider::fileName(const int id) const {
  const auto path = QString::fromStdString(taiga::get_data_path());
  const auto service = sync::serviceSlug(sync::currentServiceId());

  auto extension = u"jpg"_s;
  if (const auto item = anime::db.item(id); item && !item->image_url.empty()) {
    const QUrl url{QString::fromStdString(item->image_url)};
    if (const auto suffix = QFileInfo(url.path()).suffix(); !suffix.isEmpty()) {
      extension = suffix;
    }
  }

  return u"%1/cache/%2/media/%3.%4"_s.arg(path).arg(service).arg(id).arg(extension);
}

}  // namespace gui
