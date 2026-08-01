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
#include <QNetworkRequestFactory>
#include <QRestAccessManager>
#include <QString>

#include "media/anime_list.hpp"

namespace sync {

enum class ServiceId {
  Unknown,
  MyAnimeList,
  Kitsu,
  AniList,
};

struct Rating {
  int value = 0;
  QString text;
};

class Service : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Service)

public:
  Service(const ServiceId id);
  ~Service() = default;

  ServiceId id() const;

signals:
  void authenticationCompleted(bool authenticated);
  void searchCompleted(const QString& query, const QList<int>& ids);
  void errorOccurred(const QString& message);

protected:
  QNetworkRequestFactory api_;
  QRestAccessManager manager_;

private:
  ServiceId id_;

private slots:
  void logError(const QString& message);
  void onAuthenticationCompleted(bool authenticated);
};

ServiceId currentServiceId();
ServiceId serviceIdFromSlug(const QString& slug);
QString serviceName(const ServiceId serviceId);
QString serviceSlug(const ServiceId serviceId);
QString tagMessage(const ServiceId serviceId, const QString& message);

void authenticateUser();
void fetchAnime(const int id);
void fetchListEntries();
void search(const QString& query);
void synchronize();

void addListEntry(const int id, const anime::list::Fields dirty);
void updateListEntry(const int id, const anime::list::Fields dirty);
void deleteListEntry(const int id);

bool isUserAuthenticated();

QString animePageUrl(const int id);

}  // namespace sync
