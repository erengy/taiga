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
#include <format>

#include "base/file.hpp"
#include "base/string.hpp"
#include "compat/history.hpp"
#include "media/anime_db.hpp"
#include "media/anime_list_utils.hpp"
#include "sync/service.hpp"
#include "taiga/accounts.hpp"
#include "taiga/path.hpp"
#include "taiga/settings.hpp"

namespace sync {

Queue::Queue() : QObject{} {}

void Queue::init() {
  auto db = QSqlDatabase::database();
  if (!db.open()) return;

  const bool tableExists = db.tables().contains("queue");

  db.close();

  if (!tableExists) {
    createTable();
    migrateFromV1();
    return;
  }

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

  if (!isUserAuthenticated()) {
    emit queuedWhileUnauthenticated(animeId);
  }
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

void Queue::process() {
  if (processing_ || items_.isEmpty()) return;

  const auto item = currentItem();
  if (!item) return;

  const auto entry = anime::db.entry(item->anime_id);
  if (!entry) {
    pop(item->anime_id);
    process();
    return;
  }

  if (!isUserAuthenticated()) {
    authenticateUser();
    return;
  }

  processing_ = item->anime_id;
  emit processing(item->anime_id);

  if (entry->pending_delete) {
    deleteListEntry(item->anime_id);
  } else if (entry->id == anime::list::kUnknownId) {
    addListEntry(item->anime_id, item->dirty);
  } else {
    updateListEntry(item->anime_id, item->dirty);
  }
}

void Queue::complete(const bool success, const QString& error) {
  if (!processing_) return;

  const int animeId = *processing_;
  processing_.reset();

  if (success) {
    pop(animeId);
    process();
    return;
  }

  const auto it = findItem(animeId);
  if (it != items_.end()) {
    it->retry_count++;
    it->last_error = error.toStdString();
    persistItem(*it);
  }
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

void Queue::migrateFromV1() {
  const auto path = []() {
    const auto service = taiga::settings.service();
    return std::format("{}/v1/user/{}@{}/history.xml", taiga::get_data_path(),
                       taiga::accounts.serviceUsername(service), service);
  }();

  for (const auto& item : compat::v1::readQueue(path)) {
    if (item.delete_entry) {
      anime::list::remove(item.anime_id);
      continue;
    }

    const auto* previous = anime::db.entry(item.anime_id);
    anime::list::Entry entry = previous ? *previous : anime::list::Entry{.anime_id = item.anime_id};

    if (item.episode) entry.watched_episodes = *item.episode;
    if (item.score) entry.score = *item.score;
    if (item.status) entry.status = *item.status;
    if (item.rewatching) entry.rewatching = *item.rewatching;
    if (item.rewatched_times) entry.rewatched_times = *item.rewatched_times;
    if (item.notes) entry.notes = *item.notes;
    if (item.date_started) entry.date_started = *item.date_started;
    if (item.date_completed) entry.date_completed = *item.date_completed;

    anime::list::save(entry);
  }
}

}  // namespace sync
