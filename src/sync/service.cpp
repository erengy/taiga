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

#include "service.hpp"

#include <QMap>

#include "sync/anilist.hpp"
#include "sync/anilist_utils.hpp"
#include "sync/kitsu_utils.hpp"
#include "sync/myanimelist_utils.hpp"
#include "taiga/network.hpp"
#include "taiga/settings.hpp"

namespace sync {

Service::Service() : QObject{qApp}, manager_{taiga::network()} {
  api_.setCommonHeaders(taiga::NetworkAccessManager::commonHeaders());
}

ServiceId currentServiceId() {
  const auto slug = QString::fromStdString(taiga::settings.service());
  return serviceIdFromSlug(slug);
}

ServiceId serviceIdFromSlug(const QString& slug) {
  static const QMap<QString, ServiceId> services{
      {"myanimelist", ServiceId::MyAnimeList},
      {"kitsu", ServiceId::Kitsu},
      {"anilist", ServiceId::AniList},
  };
  return services.value(slug, ServiceId::Unknown);
}

QString serviceName(const ServiceId serviceId) {
  // clang-format off
  switch (serviceId) {
    case ServiceId::MyAnimeList: return "MyAnimeList";
    case ServiceId::Kitsu: return "Kitsu";
    case ServiceId::AniList: return "AniList";  
  }
  // clang-format on
  return "Taiga";
}

QString serviceSlug(const ServiceId serviceId) {
  // clang-format off
  switch (serviceId) {
    case ServiceId::MyAnimeList: return "myanimelist";
    case ServiceId::Kitsu: return "kitsu";
    case ServiceId::AniList: return "anilist";
  }
  // clang-format on
  return "taiga";
}

////////////////////////////////////////////////////////////////////////////////

void authenticateUser() {
  switch (currentServiceId()) {
    case ServiceId::MyAnimeList:
      break;
    case ServiceId::Kitsu:
      break;
    case ServiceId::AniList:
      anilist::Service::instance()->authenticateUser();
      break;
  }
}

void fetchAnime(const int id) {
  switch (currentServiceId()) {
    case ServiceId::MyAnimeList:
      break;
    case ServiceId::Kitsu:
      break;
    case ServiceId::AniList:
      anilist::Service::instance()->fetchAnime(id);
      break;
  }
}

QString animePageUrl(const int id) {
  switch (currentServiceId()) {
    case ServiceId::MyAnimeList:
      return QString::fromStdString(myanimelist::animePageUrl(id));
    case ServiceId::Kitsu:
      return QString::fromStdString(kitsu::animePageUrl(id));
    case ServiceId::AniList:
      return QString::fromStdString(anilist::animePageUrl(id));
  }
  return {};
}

}  // namespace sync
