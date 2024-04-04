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

#include <map>

#include "sync/service.h"

#include "taiga/settings.h"

namespace sync {

ServiceId GetCurrentServiceId() {
  return taiga::settings.GetSyncActiveService();
}

std::wstring GetCurrentServiceName() {
  return GetServiceNameById(taiga::settings.GetSyncActiveService());
}

std::wstring GetCurrentServiceSlug() {
  return GetServiceSlugById(taiga::settings.GetSyncActiveService());
}

ServiceId GetServiceIdBySlug(const std::wstring& slug) {
  static const std::map<std::wstring, ServiceId> services{
    {L"myanimelist", ServiceId::MyAnimeList},
    {L"kitsu", ServiceId::Kitsu},
    {L"anilist", ServiceId::AniList},
  };

  const auto it = services.find(slug);
  return it != services.end() ? it->second : ServiceId::Unknown;
}

std::wstring GetServiceNameById(const ServiceId service_id) {
  switch (service_id) {
    case ServiceId::MyAnimeList: return L"MyAnimeList";
    case ServiceId::Kitsu: return L"Kitsu";
    case ServiceId::AniList: return L"AniList";
    default: return L"Taiga";
  }
}

std::wstring GetServiceSlugById(const ServiceId service_id) {
  switch (service_id) {
    case ServiceId::MyAnimeList: return L"myanimelist";
    case ServiceId::Kitsu: return L"kitsu";
    case ServiceId::AniList: return L"anilist";
    default: return L"taiga";
  }
}

}  // namespace sync
