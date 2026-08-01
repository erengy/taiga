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
#include <optional>
#include <string>

#include "media/anime_list.hpp"

namespace sync {

struct QueueItem {
  int anime_id = 0;
  anime::list::Fields dirty;
  std::time_t time = 0;
  int retry_count = 0;
  std::string last_error;
};

class Queue final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Queue)

public:
  Queue();
  ~Queue() = default;

  void init();

  void push(const int animeId, const anime::list::Fields dirty);
  void pushDelete(const int animeId);
  void pop(const int animeId);

  void process();
  void complete(const bool success, const QString& error = {});

  const QueueItem* currentItem() const;
  int count() const;
  bool hasItem(const int animeId) const;

  void clear();

signals:
  void changed();

private:
  QList<QueueItem>::iterator findItem(const int animeId);
  QList<QueueItem>::const_iterator findItem(const int animeId) const;

  QString sql(const QString& name) const;

  void createTable();
  void readItems();

  void persistItem(const QueueItem& item);
  void deleteItem(const int animeId);

  void bindItemToQuery(const QueueItem& item, QSqlQuery& q) const;
  QueueItem itemFromQuery(const QSqlQuery& q) const;

  QList<QueueItem> items_;
  std::optional<int> processing_;
};

inline Queue queue;

}  // namespace sync
