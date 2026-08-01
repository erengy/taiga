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

#include "anime_db.hpp"

#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlResult>
#include <format>

#include "base/file.hpp"
#include "base/string.hpp"
#include "compat/anime.hpp"
#include "compat/list.hpp"
#include "taiga/accounts.hpp"
#include "taiga/path.hpp"
#include "taiga/settings.hpp"
#include "taiga/version.hpp"

namespace anime {

Database::Database() : QObject{} {}

void Database::init() {
  db_ = QSqlDatabase::addDatabase("QSQLITE");
  db_.setDatabaseName(fileName());

  if (!QFile::exists(fileName())) {
    createTables();
    migrateItemsFromV1();
    migrateListEntriesFromV1();
    return;
  }

  readItems();
  readEntries();
}

const Anime* Database::item(const int id) const {
  const auto it = items_.find(id);
  return it != items_.end() ? &(*it) : nullptr;
}

const ListEntry* Database::entry(const int id) const {
  const auto it = entries_.find(id);
  return it != entries_.end() ? &(*it) : nullptr;
}

const QMap<int, Anime>& Database::items() const {
  return items_;
}

const QMap<int, ListEntry>& Database::entries() const {
  return entries_;
}

void Database::updateItem(const Anime& item) {
  if (!db_.open()) return;

  QSqlQuery q{db_};
  if (!q.prepare(sql("insertAnime"))) return;
  bindItemToQuery(item, q);
  q.exec();

  db_.close();

  items_[item.id] = item;

  emit itemUpdated(item.id);
}

void Database::updateEntry(const ListEntry& entry) {
  if (!db_.open()) return;

  QSqlQuery q{db_};
  if (!q.prepare(sql("insertAnimeList"))) return;
  bindEntryToQuery(entry, q);
  q.exec();

  db_.close();

  entries_[entry.anime_id] = entry;

  emit entryUpdated(entry.anime_id);
}

void Database::deleteEntry(const int animeId) {
  if (!db_.open()) return;

  QSqlQuery q{db_};
  q.prepare("DELETE FROM anime_list WHERE media_id = :media_id");
  q.bindValue(":media_id", animeId);
  q.exec();

  db_.close();

  entries_.remove(animeId);

  emit entryDeleted(animeId);
}

QString Database::fileName() const {
  return u"%1/media.sqlite"_s.arg(QString::fromStdString(taiga::get_data_path()));
}

QString Database::sql(const QString& name) const {
  return base::readFile(u":/sql/%1.sql"_s.arg(name));
}

void Database::createTables() {
  if (!db_.open()) return;

  const auto tables = db_.tables();

  db_.transaction();

  if (!tables.contains("meta")) {
    QSqlQuery q{db_};
    q.exec(sql("createMeta"));
    q.prepare("INSERT INTO meta(name, value) VALUES(:name, :value)");
    q.bindValue(":name", "version");
    q.bindValue(":value", QString::fromStdString(taiga::version().to_string()));
    q.exec();
  }

  if (!tables.contains("anime")) {
    QSqlQuery q{db_};
    q.exec(sql("createAnime"));
  }

  if (!tables.contains("anime_list")) {
    QSqlQuery q{db_};
    q.exec(sql("createAnimeList"));
  }

  db_.commit();
  db_.close();
}

QString Database::currentVersion() {
  if (!db_.open()) return {};

  QSqlQuery q{db_};

  if (!q.prepare("SELECT value FROM meta WHERE name = :name")) return {};

  q.bindValue(":name", "version");
  q.exec();
  const QString version = q.value(0).toString();

  db_.close();

  return version;
}

void Database::readItems() {
  if (!db_.open()) return;

  QSqlQuery q{db_};
  if (!q.exec("SELECT * FROM anime")) return;

  while (q.next()) {
    const int id = q.value("id").toInt();
    items_[id] = itemFromQuery(q);
  }

  db_.close();
}

void Database::readEntries() {
  if (!db_.open()) return;

  QSqlQuery q{db_};
  if (!q.exec("SELECT * FROM anime_list")) return;

  while (q.next()) {
    const int id = q.value("media_id").toInt();
    entries_[id] = entryFromQuery(q);
  }

  db_.close();
}

void Database::bindItemToQuery(const Anime& item, QSqlQuery& q) const {
  q.bindValue(":id", item.id);
  q.bindValue(":title", QString::fromStdString(item.titles.romaji));
  q.bindValue(":english", QString::fromStdString(item.titles.english));
  q.bindValue(":japanese", QString::fromStdString(item.titles.japanese));
  q.bindValue(":synonym", joinStrings(item.titles.synonyms, ""));
  q.bindValue(":type", static_cast<int>(item.type));
  q.bindValue(":status", static_cast<int>(item.status));
  q.bindValue(":episode_count", item.episode_count);
  q.bindValue(":episode_length", item.episode_length);
  q.bindValue(":date_start", QString::fromStdString(item.date_started.to_string()));
  q.bindValue(":date_end", QString::fromStdString(item.date_finished.to_string()));
  q.bindValue(":image", QString::fromStdString(item.image_url));
  q.bindValue(":trailer_id", QString::fromStdString(item.trailer_id));
  q.bindValue(":age_rating", static_cast<int>(item.age_rating));
  q.bindValue(":genres", joinStrings(item.genres, ""));
  q.bindValue(":tags", joinStrings(item.tags, ""));
  q.bindValue(":producers", joinStrings(item.producers, ""));
  q.bindValue(":studios", joinStrings(item.studios, ""));
  q.bindValue(":score", QString::number(item.score));
  q.bindValue(":popularity", item.popularity_rank);
  q.bindValue(":synopsis", QString::fromStdString(item.synopsis));
  q.bindValue(":last_aired_episode", item.last_aired_episode);
  q.bindValue(":next_episode_time", QString::number(item.next_episode_time));
  q.bindValue(":modified", QString::number(item.last_modified));
}

void Database::bindEntryToQuery(const ListEntry& entry, QSqlQuery& q) const {
  q.bindValue(":id", entry.id);
  q.bindValue(":media_id", entry.anime_id);
  q.bindValue(":progress", entry.watched_episodes);
  q.bindValue(":date_start", QString::fromStdString(entry.date_started.to_string()));
  q.bindValue(":date_end", QString::fromStdString(entry.date_completed.to_string()));
  q.bindValue(":score", entry.score);
  q.bindValue(":status", static_cast<int>(entry.status));
  q.bindValue(":private", entry.is_private);
  q.bindValue(":rewatched_times", entry.rewatched_times);
  q.bindValue(":rewatching", entry.rewatching);
  q.bindValue(":rewatching_ep", entry.rewatching_ep);
  q.bindValue(":notes", QString::fromStdString(entry.notes));
  q.bindValue(":last_updated", QString::number(entry.last_updated));
  q.bindValue(":pending_delete", entry.pending_delete);
}

Anime Database::itemFromQuery(const QSqlQuery& q) const {
  static const auto splitToVector = [](const QVariant& variant) {
    return toVector(variant.toString().split(", ", Qt::SkipEmptyParts));
  };
  return {
      .id = q.value("id").toInt(),
      .last_modified = q.value("modified").toInt(),
      .episode_count = q.value("episode_count").toInt(),
      .episode_length = q.value("episode_length").toInt(),
      .age_rating = q.value("age_rating").value<anime::AgeRating>(),
      .status = q.value("status").value<anime::Status>(),
      .type = q.value("type").value<anime::Type>(),
      .date_started = FuzzyDate(q.value("date_start").toString().toStdString()),
      .date_finished = FuzzyDate(q.value("date_end").toString().toStdString()),
      .score = q.value("score").toFloat(),
      .popularity_rank = q.value("popularity").toInt(),
      .image_url = q.value("image").toString().toStdString(),
      .synopsis = q.value("synopsis").toString().toStdString(),
      .trailer_id = q.value("trailer_id").toString().toStdString(),
      .titles{
          .romaji = q.value("title").toString().toStdString(),
          .english = q.value("english").toString().toStdString(),
          .japanese = q.value("japanese").toString().toStdString(),
          .synonyms = splitToVector(q.value("synonym")),
      },
      .genres = splitToVector(q.value("genres")),
      .producers = splitToVector(q.value("producers")),
      .studios = splitToVector(q.value("studios")),
      .tags = splitToVector(q.value("tags")),
      .last_aired_episode = q.value("last_aired_episode").toInt(),
      .next_episode_time = q.value("next_episode_time").toInt(),
  };
}

ListEntry Database::entryFromQuery(const QSqlQuery& q) const {
  return {
      .id = q.value("id").toLongLong(),
      .anime_id = q.value("media_id").toInt(),
      .watched_episodes = q.value("progress").toInt(),
      .score = q.value("score").toInt(),
      .status = q.value("status").value<anime::list::Status>(),
      .is_private = q.value("private").toBool(),
      .rewatched_times = q.value("rewatched_times").toInt(),
      .rewatching = q.value("rewatching").toBool(),
      .rewatching_ep = q.value("rewatching_ep").toInt(),
      .date_started = FuzzyDate(q.value("date_start").toString().toStdString()),
      .date_completed = FuzzyDate(q.value("date_end").toString().toStdString()),
      .last_updated = q.value("last_updated").toInt(),
      .notes = q.value("notes").toString().toStdString(),
      .pending_delete = q.value("pending_delete").toBool(),
  };
}

void Database::migrateItemsFromV1() {
  if (!db_.open()) return;

  QSqlQuery q{db_};
  if (!q.prepare(sql("insertAnime"))) return;

  const auto path = std::format("{}/v1/db/anime.xml", taiga::get_data_path());

  db_.transaction();

  for (const auto& item : compat::v1::readAnimeDatabase(path)) {
    items_[item.id] = item;
    bindItemToQuery(item, q);
    q.exec();
  }

  db_.commit();
  db_.close();
}

void Database::migrateListEntriesFromV1() {
  if (!db_.open()) return;

  QSqlQuery q{db_};
  if (!q.prepare(sql("insertAnimeList"))) return;

  const auto path = []() {
    const auto service = taiga::settings.service();
    return std::format("{}/v1/user/{}@{}/anime.xml", taiga::get_data_path(),
                       taiga::accounts.serviceUsername(service), service);
  }();

  db_.transaction();

  for (const auto& entry : compat::v1::readListEntries(path)) {
    if (!items_.contains(entry.anime_id)) continue;
    entries_[entry.anime_id] = entry;
    bindEntryToQuery(entry, q);
    q.exec();
  }

  db_.commit();
  db_.close();
}

}  // namespace anime
