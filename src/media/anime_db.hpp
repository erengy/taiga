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
#include <QSqlDatabase>
#include <QSqlQuery>

#include "media/anime.hpp"
#include "media/anime_list.hpp"
#include "media/anime_settings.hpp"

namespace anime {

class Database final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Database)

public:
  Database();
  ~Database() = default;

  void init();

  const Anime* item(const int id) const;
  const ListEntry* entry(const int id) const;
  const Settings* settings(const int id) const;

  const QMap<int, Anime>& items() const;
  const QMap<int, ListEntry>& entries() const;

  void updateItem(const Anime& item);
  void updateItems(const QList<Anime>& items);
  void updateEntry(const ListEntry& entry);
  void updateEntries(const QList<ListEntry>& entries);
  void deleteItem(const int id);
  void deleteEntry(const int animeId);
  void updateSettings(const Settings& settings);

signals:
  void itemUpdated(const int id);
  void entryUpdated(const int id);
  void itemDeleted(const int id);
  void entryDeleted(const int id);

private:
  QString fileName() const;
  QString sql(const QString& name) const;

  void createTables();
  QString currentVersion();

  void readItems();
  void readEntries();
  void readSettings();

  void bindItemToQuery(const Anime& item, QSqlQuery& q) const;
  void bindEntryToQuery(const ListEntry& entry, QSqlQuery& q) const;
  void bindSettingsToQuery(const Settings& settings, QSqlQuery& q) const;

  Anime itemFromQuery(const QSqlQuery& q) const;
  ListEntry entryFromQuery(const QSqlQuery& q) const;
  Settings settingsFromQuery(const QSqlQuery& q) const;

  void migrateItemsFromV1();
  void migrateListEntriesFromV1();
  void migrateSettingsFromV1();

  QSqlDatabase db_;

  QMap<int, Anime> items_;
  QMap<int, ListEntry> entries_;
  QMap<int, Settings> settings_;
};

inline Database db;

}  // namespace anime
