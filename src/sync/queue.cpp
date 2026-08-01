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

#include "queue.hpp"

#include <QSqlDatabase>
#include <algorithm>

#include "base/file.hpp"
#include "base/string.hpp"

namespace sync {

Queue::Queue() : QObject{} {}

void Queue::init() {
  createTable();
  readItems();
}

void Queue::push(const int animeId, const anime::list::Fields dirty) {
  const auto it = findItem(animeId);

  if (it != items_.end()) {
    it->dirty |= dirty;
    it->time = std::time(nullptr);
    persistItem(*it);
  } else {
    QueueItem item{
        .anime_id = animeId,
        .dirty = dirty,
        .time = std::time(nullptr),
    };
    items_.append(item);
    persistItem(item);
  }

  emit changed();
}

void Queue::pushDelete(const int animeId) {
  pop(animeId);
  push(animeId, {});
}

void Queue::pop(const int animeId) {
  const auto it = findItem(animeId);
  if (it == items_.end()) return;

  items_.erase(it);
  deleteItem(animeId);

  emit changed();
}

const QueueItem* Queue::currentItem() const {
  if (items_.isEmpty()) return nullptr;

  const auto it = std::ranges::min_element(items_, {}, &QueueItem::time);
  return &(*it);
}

int Queue::count() const {
  return items_.size();
}

bool Queue::hasItem(const int animeId) const {
  return findItem(animeId) != items_.end();
}

void Queue::clear() {
  auto db = QSqlDatabase::database();
  if (db.open()) {
    QSqlQuery q{db};
    q.exec("DELETE FROM queue");
    db.close();
  }

  items_.clear();

  emit changed();
}

QList<QueueItem>::iterator Queue::findItem(const int animeId) {
  return std::ranges::find(items_, animeId, &QueueItem::anime_id);
}

QList<QueueItem>::const_iterator Queue::findItem(const int animeId) const {
  return std::ranges::find(items_, animeId, &QueueItem::anime_id);
}

QString Queue::sql(const QString& name) const {
  return base::readFile(u":/sql/%1.sql"_s.arg(name));
}

void Queue::createTable() {
  auto db = QSqlDatabase::database();
  if (!db.open()) return;

  if (!db.tables().contains("queue")) {
    QSqlQuery q{db};
    q.exec(sql("createQueue"));
  }

  db.close();
}

void Queue::readItems() {
  auto db = QSqlDatabase::database();
  if (!db.open()) return;

  QSqlQuery q{db};
  if (q.exec("SELECT * FROM queue ORDER BY time ASC")) {
    while (q.next()) {
      items_.append(itemFromQuery(q));
    }
  }

  db.close();
}

void Queue::persistItem(const QueueItem& item) {
  auto db = QSqlDatabase::database();
  if (!db.open()) return;

  QSqlQuery q{db};
  if (q.prepare(sql("insertQueue"))) {
    bindItemToQuery(item, q);
    q.exec();
  }

  db.close();
}

void Queue::deleteItem(const int animeId) {
  auto db = QSqlDatabase::database();
  if (!db.open()) return;

  QSqlQuery q{db};
  q.prepare("DELETE FROM queue WHERE anime_id = :anime_id");
  q.bindValue(":anime_id", animeId);
  q.exec();

  db.close();
}

void Queue::bindItemToQuery(const QueueItem& item, QSqlQuery& q) const {
  q.bindValue(":anime_id", item.anime_id);
  q.bindValue(":dirty", item.dirty.toInt());
  q.bindValue(":time", static_cast<qlonglong>(item.time));
  q.bindValue(":retry_count", item.retry_count);
  q.bindValue(":last_error", item.last_error.empty()
                                 ? QVariant()
                                 : QVariant(QString::fromStdString(item.last_error)));
}

QueueItem Queue::itemFromQuery(const QSqlQuery& q) const {
  QueueItem item{
      .anime_id = q.value("anime_id").toInt(),
      .dirty = anime::list::Fields::fromInt(q.value("dirty").toInt()),
      .time = static_cast<std::time_t>(q.value("time").toLongLong()),
      .retry_count = q.value("retry_count").toInt(),
  };

  if (!q.value("last_error").isNull()) {
    item.last_error = q.value("last_error").toString().toStdString();
  }

  return item;
}

}  // namespace sync
