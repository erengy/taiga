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

#include <QList>
#include <QSqlQuery>
#include <ctime>

namespace anime {

struct HistoryItem {
  int id = 0;
  int anime_id = 0;
  int episode = 0;
  std::time_t time = 0;
};

class History final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(History)

public:
  History();
  ~History() = default;

  void init();

  void add(const int animeId, const int episode, const std::time_t time);
  void clear();

  const QList<HistoryItem>& items() const;

signals:
  void changed();

private:
  QString sql(const QString& name) const;

  void createTable();
  void readItems();

  void bindItemToQuery(const HistoryItem& item, QSqlQuery& q) const;
  HistoryItem itemFromQuery(const QSqlQuery& q) const;

  void migrateFromV1();

  QList<HistoryItem> items_;
};

inline History history;

}  // namespace anime
