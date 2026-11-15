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

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QImage>
#include <QImageReader>
#include <QNetworkRequest>
#include <QPixmapCache>
#include <QRestReply>
#include <QUrl>
#include <QtConcurrentRun>

#include "base/string.hpp"
#include "media/anime_db.hpp"
#include "media/anime_utils.hpp"
#include "sync/service.hpp"
#include "taiga/network.hpp"
#include "taiga/path.hpp"

namespace gui {

namespace {

constexpr int kPixmapCacheLimitKb = 200 * 1024;  // 200MB

QString cacheKey(const int id) {
  return u"poster/%1"_s.arg(id);
}

}  // namespace

ImageProvider::ImageProvider(QObject* parent) : QObject{parent} {
  QPixmapCache::setCacheLimit(kPixmapCacheLimitKb);
}

void ImageProvider::init() {
  manager_ = new QRestAccessManager(taiga::network(), this);
}

void ImageProvider::fetchPoster(const int id, const bool revalidate) {
  const auto item = anime::db()->item(id);

  if (!item || item->imageUrl.empty()) return;

  QNetworkRequest request{QString::fromStdString(item->imageUrl)};

  if (revalidate) {
    if (const QFileInfo file{fileName(id)}; file.exists()) {
      request.setHeader(QNetworkRequest::IfModifiedSinceHeader, file.lastModified());
    }
  }

  manager_->get(request, this, [this, id](QRestReply& reply) {
    if (!reply.isHttpStatusSuccess() || reply.hasError()) {
      if (reply.httpStatus() == 404) {
        if (const auto item = anime::db()->item(id)) {
          auto updatedItem = *item;
          updatedItem.imageUrl.clear();
          anime::db()->updateItem(updatedItem);
        }
      }
      return;
    }

    QFile file{fileName(id)};
    QDir().mkpath(QFileInfo(file).path());
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(reply.readBody());
    retryAfter_.remove(id);
    reloadPoster(id);
  });
}

QPixmap ImageProvider::loadPoster(const int id) {
  if (QPixmap pixmap; QPixmapCache::find(cacheKey(id), &pixmap)) {
    return pixmap;
  }

  if (loading_.contains(id) || !canRetry(id)) {
    return QPixmap{};
  }

  loading_.insert(id);

  const auto future = QtConcurrent::run([fileName = fileName(id)] {
    QImageReader reader(fileName);
    return reader.read();
  });

  const auto watcher = new QFutureWatcher<QImage>(this);
  connect(watcher, &QFutureWatcherBase::finished, this, [this, id, watcher]() {
    watcher->deleteLater();
    loading_.remove(id);

    const QImage image = watcher->result();

    if (image.isNull()) {
      retryAfter(id);
      fetchPoster(id);
    } else {
      QPixmapCache::insert(cacheKey(id), QPixmap::fromImage(image));
      if (isStale(id)) fetchPoster(id, true);
    }

    emit posterChanged(id);
  });
  watcher->setFuture(future);

  return QPixmap{};
}

void ImageProvider::reloadPoster(const int id) {
  QPixmapCache::remove(cacheKey(id));
  loadPoster(id);
}

QString ImageProvider::fileName(const int id) const {
  const auto path = QString::fromStdString(taiga::getDataPath());
  const auto service = sync::serviceSlug(sync::currentServiceId());

  auto extension = u"jpg"_s;
  if (const auto item = anime::db()->item(id); item && !item->imageUrl.empty()) {
    const QUrl url{QString::fromStdString(item->imageUrl)};
    if (const auto suffix = QFileInfo(url.path()).suffix(); !suffix.isEmpty()) {
      extension = suffix;
    }
  }

  return u"%1/cache/%2/media/%3.%4"_s.arg(path).arg(service).arg(id).arg(extension);
}

bool ImageProvider::isStale(const int id) const {
  constexpr int kStaleDays = 7;

  const auto item = anime::db()->item(id);

  if (!item) return false;

  if (anime::airingStatus(*item) == anime::Status::FinishedAiring) {
    return false;
  }

  const QFileInfo file{fileName(id)};
  return file.lastModified().daysTo(QDateTime::currentDateTime()) >= kStaleDays;
}

bool ImageProvider::canRetry(const int id) const {
  const auto it = retryAfter_.find(id);
  return it == retryAfter_.end() || QDateTime::currentDateTime() >= it.value();
}

void ImageProvider::retryAfter(const int id) {
  constexpr int kRetryCooldownSecs = 60;
  retryAfter_[id] = QDateTime::currentDateTime().addSecs(kRetryCooldownSecs);
}

ImageProvider* imageProvider() {
  static auto instance = new ImageProvider{qApp};
  return instance;
}

}  // namespace gui
