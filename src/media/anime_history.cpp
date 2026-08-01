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

#include "anime_history.hpp"

#include <QSqlDatabase>
#include <algorithm>
#include <format>

#include "base/file.hpp"
#include "base/string.hpp"
#include "compat/history.hpp"
#include "taiga/accounts.hpp"
#include "taiga/path.hpp"
#include "taiga/settings.hpp"

namespace anime {

History::History() : QObject{} {}

void History::init() {
  auto db = QSqlDatabase::database();
  if (!db.open()) return;

  const bool tableExists = db.tables().contains("history");

  db.close();

  if (!tableExists) {
    createTable();
    migrateFromV1();
    return;
  }

  readItems();
}

void History::add(const int animeId, const int episode, const std::time_t time) {
  auto db = QSqlDatabase::database();
  if (!db.open()) return;

  QSqlQuery q{db};
  int id = 0;
  if (q.prepare(sql("insertHistory"))) {
    q.bindValue(":anime_id", animeId);
    q.bindValue(":episode", episode);
    q.bindValue(":time", static_cast<qlonglong>(time));
    q.exec();
    id = q.lastInsertId().toInt();
  }

  db.close();

  items_.append(HistoryItem{
      .id = id,
      .anime_id = animeId,
      .episode = episode,
      .time = time,
  });

  emit changed();
}

void History::remove(const int id) {
  auto db = QSqlDatabase::database();
  if (db.open()) {
    QSqlQuery q{db};
    q.prepare("DELETE FROM history WHERE id = :id");
    q.bindValue(":id", id);
    q.exec();
    db.close();
  }

  const auto it = std::ranges::find(items_, id, &HistoryItem::id);
  if (it != items_.end()) items_.erase(it);

  emit changed();
}

void History::clear() {
  auto db = QSqlDatabase::database();
  if (db.open()) {
    QSqlQuery q{db};
    q.exec("DELETE FROM history");
    db.close();
  }

  items_.clear();

  emit changed();
}

const QList<HistoryItem>& History::items() const {
  return items_;
}

QString History::sql(const QString& name) const {
  return base::readFile(u":/sql/%1.sql"_s.arg(name));
}

void History::createTable() {
  auto db = QSqlDatabase::database();
  if (!db.open()) return;

  if (!db.tables().contains("history")) {
    QSqlQuery q{db};
    q.exec(sql("createHistory"));
  }

  db.close();
}

void History::readItems() {
  auto db = QSqlDatabase::database();
  if (!db.open()) return;

  QSqlQuery q{db};
  if (q.exec("SELECT * FROM history ORDER BY id ASC")) {
    while (q.next()) {
      items_.append(itemFromQuery(q));
    }
  }

  db.close();
}

void History::bindItemToQuery(const HistoryItem& item, QSqlQuery& q) const {
  q.bindValue(":anime_id", item.anime_id);
  q.bindValue(":episode", item.episode);
  q.bindValue(":time", static_cast<qlonglong>(item.time));
}

HistoryItem History::itemFromQuery(const QSqlQuery& q) const {
  return {
      .id = q.value("id").toInt(),
      .anime_id = q.value("anime_id").toInt(),
      .episode = q.value("episode").toInt(),
      .time = static_cast<std::time_t>(q.value("time").toLongLong()),
  };
}

void History::migrateFromV1() {
  auto db = QSqlDatabase::database();
  if (!db.open()) return;

  QSqlQuery q{db};
  if (!q.prepare(sql("insertHistory"))) return;

  const auto path = []() {
    const auto service = taiga::settings.service();
    return std::format("{}/v1/user/{}@{}/history.xml", taiga::get_data_path(),
                       taiga::accounts.serviceUsername(service), service);
  }();

  db.transaction();

  for (auto item : compat::v1::readHistory(path)) {
    bindItemToQuery(item, q);
    q.exec();
    item.id = q.lastInsertId().toInt();
    items_.append(item);
  }

  db.commit();
  db.close();
}

}  // namespace anime
