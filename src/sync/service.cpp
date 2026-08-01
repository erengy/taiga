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

#include "service.hpp"

#include <QMap>

#include "base/log.hpp"
#include "base/string.hpp"
#include "sync/anilist.hpp"
#include "sync/anilist_utils.hpp"
#include "sync/kitsu_utils.hpp"
#include "sync/myanimelist_utils.hpp"
#include "sync/queue.hpp"
#include "taiga/accounts.hpp"
#include "taiga/network.hpp"
#include "taiga/settings.hpp"

namespace sync {

Service::Service(const ServiceId id) : QObject{qApp}, manager_{taiga::network()}, id_{id} {
  api_.setCommonHeaders(taiga::NetworkAccessManager::commonHeaders());

  connect(this, &Service::authenticationCompleted, this, &Service::onAuthenticationCompleted);
  connect(this, &Service::errorOccurred, this, &Service::logError);
}

ServiceId Service::id() const {
  return id_;
}

void Service::logError(const QString& message) {
  LOGE("{}", tagMessage(id_, message).toStdString());
}

void Service::onAuthenticationCompleted(bool authenticated) {
  switch (currentServiceId()) {
    case ServiceId::MyAnimeList:
      taiga::accounts.setMyanimelistAuthenticated(authenticated);
      break;
    case ServiceId::Kitsu:
      taiga::accounts.setKitsuAuthenticated(authenticated);
      break;
    case ServiceId::AniList:
      taiga::accounts.setAnilistAuthenticated(authenticated);
      break;
  }

  if (authenticated) {
    synchronize();
  }
}

////////////////////////////////////////////////////////////////////////////////

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

QString tagMessage(const ServiceId serviceId, const QString& message) {
  return u"[%1] %2"_s.arg(serviceName(serviceId), message);
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

void fetchListEntries() {
  switch (currentServiceId()) {
    case ServiceId::MyAnimeList:
      break;
    case ServiceId::Kitsu:
      break;
    case ServiceId::AniList:
      anilist::Service::instance()->fetchListEntries();
      break;
  }
}

void search(const QString& query) {
  switch (currentServiceId()) {
    case ServiceId::MyAnimeList:
      break;
    case ServiceId::Kitsu:
      break;
    case ServiceId::AniList:
      anilist::Service::instance()->search(query);
      break;
  }
}

void synchronize() {
  switch (currentServiceId()) {
    case ServiceId::MyAnimeList:
      break;

    case ServiceId::Kitsu:
      break;

    case ServiceId::AniList:
      if (!taiga::accounts.anilistAuthenticated()) {
        if (!taiga::accounts.anilistToken().empty()) {
          authenticateUser();
        } else if (!taiga::accounts.anilistUsername().empty()) {
          // Allow downloading the list without authentication
          fetchListEntries();
        }
        return;
      }

      if (queue.count() > 0) {
        queue.process();
      } else {
        fetchListEntries();
      }
      break;
  }
}

void addListEntry(const int id, const anime::list::Fields dirty) {
  switch (currentServiceId()) {
    case ServiceId::MyAnimeList:
      break;
    case ServiceId::Kitsu:
      break;
    case ServiceId::AniList:
      anilist::Service::instance()->addListEntry(id, dirty);
      break;
  }
}

void updateListEntry(const int id, const anime::list::Fields dirty) {
  switch (currentServiceId()) {
    case ServiceId::MyAnimeList:
      break;
    case ServiceId::Kitsu:
      break;
    case ServiceId::AniList:
      anilist::Service::instance()->updateListEntry(id, dirty);
      break;
  }
}

void deleteListEntry(const int id) {
  switch (currentServiceId()) {
    case ServiceId::MyAnimeList:
      break;
    case ServiceId::Kitsu:
      break;
    case ServiceId::AniList:
      anilist::Service::instance()->deleteListEntry(id);
      break;
  }
}

bool isUserAuthenticated() {
  switch (currentServiceId()) {
    case ServiceId::MyAnimeList:
      return taiga::accounts.myanimelistAuthenticated();
    case ServiceId::Kitsu:
      return taiga::accounts.kitsuAuthenticated();
    case ServiceId::AniList:
      return taiga::accounts.anilistAuthenticated();
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////

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
