/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
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

#include <QNetworkRequestFactory>
#include <QRestAccessManager>
#include <QString>

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
public:
  Service();
  ~Service() = default;

protected:
  QNetworkRequestFactory api_;
  QRestAccessManager manager_;
};

ServiceId currentServiceId();
ServiceId serviceIdFromSlug(const QString& slug);
QString serviceName(const ServiceId serviceId);
QString serviceSlug(const ServiceId serviceId);

void authenticateUser();
void fetchAnime(const int id);

QString animePageUrl(const int id);

}  // namespace sync
