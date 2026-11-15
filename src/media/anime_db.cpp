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

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlResult>
#include <format>

#include "base/file.hpp"
#include "base/string.hpp"
#include "compat/anime.hpp"
#include "compat/list.hpp"
#include "compat/settings.hpp"
#include "media/anime_utils.hpp"
#include "taiga/accounts.hpp"
#include "taiga/path.hpp"
#include "taiga/settings.hpp"
#include "taiga/version.hpp"

namespace {

std::vector<std::string> fromJsonArray(const QVariant& variant) {
  const auto array = QJsonDocument::fromJson(variant.toString().toUtf8()).array();
  std::vector<std::string> list;
  list.reserve(array.size());
  for (const auto& value : array) {
    list.push_back(value.toString().toStdString());
  }
  return list;
}

QString toJsonArray(const std::vector<std::string>& list) {
  QJsonArray array;
  for (const auto& item : list) {
    array.append(QString::fromStdString(item));
  }
  return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

}  // namespace

namespace anime {

Database::Database(QObject* parent) : QObject{parent} {}

void Database::init() {
  db_ = QSqlDatabase::addDatabase("QSQLITE");
  db_.setDatabaseName(fileName());

  if (!QFile::exists(fileName())) {
    createTables();
    migrateItemsFromV1();
    migrateListEntriesFromV1();
    migrateSettingsFromV1();
    return;
  }

  readItems();
  readEntries();
  readSettings();
}

const Anime* Database::item(const int id) const {
  const auto it = items_.find(id);
  return it != items_.end() ? &(*it) : nullptr;
}

const ListEntry* Database::entry(const int id) const {
  const auto it = entries_.find(id);
  return it != entries_.end() ? &(*it) : nullptr;
}

const Settings* Database::settings(const int id) const {
  const auto it = settings_.find(id);
  return it != settings_.end() ? &(*it) : nullptr;
}

const QMap<int, Anime>& Database::items() const {
  return items_;
}

const QMap<int, ListEntry>& Database::entries() const {
  return entries_;
}

void Database::updateItem(const Anime& item) {
  updateItems({item});
}

void Database::updateItems(const QList<Anime>& items) {
  if (items.isEmpty()) return;
  if (!db_.open()) return;

  QSqlQuery q{db_};
  if (!q.prepare(sql("insertAnime"))) {
    db_.close();
    return;
  }

  db_.transaction();
  for (const auto& item : items) {
    bindItemToQuery(item, q);
    q.exec();
  }
  db_.commit();

  db_.close();

  for (const auto& item : items) {
    items_[item.id] = item;
  }
  for (const auto& item : items) {
    emit itemUpdated(item.id);
  }
}

void Database::updateEntry(const ListEntry& entry) {
  updateEntries({entry});
}

void Database::updateEntries(const QList<ListEntry>& entries) {
  if (entries.isEmpty()) return;
  if (!db_.open()) return;

  QSqlQuery q{db_};
  if (!q.prepare(sql("insertAnimeList"))) {
    db_.close();
    return;
  }

  db_.transaction();
  for (const auto& entry : entries) {
    bindEntryToQuery(entry, q);
    q.exec();
  }
  db_.commit();

  db_.close();

  for (const auto& entry : entries) {
    entries_[entry.animeId] = entry;
  }
  for (const auto& entry : entries) {
    emit entryUpdated(entry.animeId);
  }
}

void Database::updateSettings(const Settings& settings) {
  if (!db_.open()) return;

  QSqlQuery q{db_};
  q.prepare(sql("insertAnimeSettings"));
  bindSettingsToQuery(settings, q);
  q.exec();

  db_.close();

  settings_[settings.id] = settings;

  emit itemUpdated(settings.id);
}

void Database::deleteItem(const int id) {
  const auto* existing = item(id);
  const auto title = existing ? QString::fromStdString(preferredTitle(*existing)) : QString();

  if (!db_.open()) return;

  QSqlQuery q{db_};
  q.prepare("DELETE FROM anime WHERE id = :id");
  q.bindValue(":id", id);
  q.exec();

  db_.close();

  items_.remove(id);

  emit itemDeleted(id, title);
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
  return u"%1/media.sqlite"_s.arg(taiga::getDataPath());
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

  if (!tables.contains("anime_settings")) {
    QSqlQuery q{db_};
    q.exec(sql("createAnimeSettings"));
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

void Database::readSettings() {
  if (!db_.open()) return;

  QSqlQuery q{db_};

  if (!q.exec("SELECT * FROM anime_settings")) {
    db_.close();
    return;
  }

  while (q.next()) {
    const int id = q.value("id").toInt();
    settings_[id] = settingsFromQuery(q);
  }

  db_.close();
}

void Database::bindItemToQuery(const Anime& item, QSqlQuery& q) const {
  q.bindValue(":id", item.id);
  q.bindValue(":title", QString::fromStdString(item.titles.romaji));
  q.bindValue(":english", QString::fromStdString(item.titles.english));
  q.bindValue(":japanese", QString::fromStdString(item.titles.japanese));
  q.bindValue(":synonym", toJsonArray(item.titles.synonyms));
  q.bindValue(":type", static_cast<int>(item.type));
  q.bindValue(":status", static_cast<int>(item.status));
  q.bindValue(":episode_count", item.episodeCount);
  q.bindValue(":episode_length", item.episodeLength);
  q.bindValue(":date_start", QString::fromStdString(item.dateStarted.toString()));
  q.bindValue(":date_end", QString::fromStdString(item.dateFinished.toString()));
  q.bindValue(":image", QString::fromStdString(item.imageUrl));
  q.bindValue(":trailer_id", QString::fromStdString(item.trailerId));
  q.bindValue(":age_rating", static_cast<int>(item.ageRating));
  q.bindValue(":genres", toJsonArray(item.genres));
  q.bindValue(":tags", toJsonArray(item.tags));
  q.bindValue(":producers", toJsonArray(item.producers));
  q.bindValue(":studios", toJsonArray(item.studios));
  q.bindValue(":score", QString::number(item.score));
  q.bindValue(":popularity", item.popularityRank);
  q.bindValue(":synopsis", QString::fromStdString(item.synopsis));
  q.bindValue(":last_aired_episode", item.lastAiredEpisode);
  q.bindValue(":next_episode_time", QString::number(item.nextEpisodeTime));
  q.bindValue(":modified", QString::number(item.lastModified));
}

void Database::bindEntryToQuery(const ListEntry& entry, QSqlQuery& q) const {
  q.bindValue(":id", static_cast<qint64>(entry.id));
  q.bindValue(":media_id", entry.animeId);
  q.bindValue(":progress", entry.watchedEpisodes);
  q.bindValue(":date_start", QString::fromStdString(entry.dateStarted.toString()));
  q.bindValue(":date_end", QString::fromStdString(entry.dateCompleted.toString()));
  q.bindValue(":score", entry.score);
  q.bindValue(":status", static_cast<int>(entry.status));
  q.bindValue(":private", entry.isPrivate);
  q.bindValue(":rewatched_times", entry.rewatchedTimes);
  q.bindValue(":rewatching", entry.rewatching);
  q.bindValue(":rewatching_ep", entry.rewatchingEp);
  q.bindValue(":notes", QString::fromStdString(entry.notes));
  q.bindValue(":last_updated", QString::number(entry.lastUpdated));
  q.bindValue(":pending_delete", entry.pendingDelete);
}

void Database::bindSettingsToQuery(const Settings& settings, QSqlQuery& q) const {
  q.bindValue(":id", settings.id);
  q.bindValue(":display_title", QString::fromStdString(settings.displayTitle));
  q.bindValue(":folder", QString::fromStdString(settings.folder));
  q.bindValue(":synonyms", toJsonArray(settings.synonyms));
}

Anime Database::itemFromQuery(const QSqlQuery& q) const {
  return {
      .id = q.value("id").toInt(),
      .lastModified = q.value("modified").toInt(),
      .episodeCount = q.value("episode_count").toInt(),
      .episodeLength = q.value("episode_length").toInt(),
      .ageRating = q.value("age_rating").value<anime::AgeRating>(),
      .status = q.value("status").value<anime::Status>(),
      .type = q.value("type").value<anime::Type>(),
      .dateStarted = FuzzyDate(q.value("date_start").toString().toStdString()),
      .dateFinished = FuzzyDate(q.value("date_end").toString().toStdString()),
      .score = q.value("score").toFloat(),
      .popularityRank = q.value("popularity").toInt(),
      .imageUrl = q.value("image").toString().toStdString(),
      .synopsis = q.value("synopsis").toString().toStdString(),
      .trailerId = q.value("trailer_id").toString().toStdString(),
      .titles{
          .romaji = q.value("title").toString().toStdString(),
          .english = q.value("english").toString().toStdString(),
          .japanese = q.value("japanese").toString().toStdString(),
          .synonyms = fromJsonArray(q.value("synonym")),
      },
      .genres = fromJsonArray(q.value("genres")),
      .producers = fromJsonArray(q.value("producers")),
      .studios = fromJsonArray(q.value("studios")),
      .tags = fromJsonArray(q.value("tags")),
      .lastAiredEpisode = q.value("last_aired_episode").toInt(),
      .nextEpisodeTime = q.value("next_episode_time").toInt(),
  };
}

ListEntry Database::entryFromQuery(const QSqlQuery& q) const {
  return {
      .id = q.value("id").toLongLong(),
      .animeId = q.value("media_id").toInt(),
      .watchedEpisodes = q.value("progress").toInt(),
      .score = q.value("score").toInt(),
      .status = q.value("status").value<anime::list::Status>(),
      .isPrivate = q.value("private").toBool(),
      .rewatchedTimes = q.value("rewatched_times").toInt(),
      .rewatching = q.value("rewatching").toBool(),
      .rewatchingEp = q.value("rewatching_ep").toInt(),
      .dateStarted = FuzzyDate(q.value("date_start").toString().toStdString()),
      .dateCompleted = FuzzyDate(q.value("date_end").toString().toStdString()),
      .lastUpdated = q.value("last_updated").toInt(),
      .notes = q.value("notes").toString().toStdString(),
      .pendingDelete = q.value("pending_delete").toBool(),
  };
}

Settings Database::settingsFromQuery(const QSqlQuery& q) const {
  return {
      .id = q.value("id").toInt(),
      .displayTitle = q.value("display_title").toString().toStdString(),
      .folder = q.value("folder").toString().toStdString(),
      .synonyms = fromJsonArray(q.value("synonyms")),
  };
}

void Database::migrateItemsFromV1() {
  if (!db_.open()) return;

  QSqlQuery q{db_};
  if (!q.prepare(sql("insertAnime"))) return;

  const auto path = std::format("{}/v1/db/anime.xml", taiga::getDataPath());

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
    const auto service = taiga::settings()->syncService();
    return std::format("{}/v1/user/{}@{}/anime.xml", taiga::getDataPath(),
                       taiga::accounts()->serviceUsername(service), service);
  }();

  db_.transaction();

  for (const auto& entry : compat::v1::readListEntries(path)) {
    if (!items_.contains(entry.animeId)) continue;
    entries_[entry.animeId] = entry;
    bindEntryToQuery(entry, q);
    q.exec();
  }

  db_.commit();
  db_.close();
}

void Database::migrateSettingsFromV1() {
  if (!db_.open()) return;

  QSqlQuery q{db_};
  if (!q.prepare(sql("insertAnimeSettings"))) return;

  const auto path = std::format("{}/v1/settings.xml", taiga::getDataPath());

  db_.transaction();

  for (const auto& settings : compat::v1::readAnimeSettings(path)) {
    if (!items_.contains(settings.id)) continue;
    settings_[settings.id] = settings;
    bindSettingsToQuery(settings, q);
    q.exec();
  }

  db_.commit();
  db_.close();
}

Database* db() {
  static auto instance = new Database{qApp};
  return instance;
}

}  // namespace anime
