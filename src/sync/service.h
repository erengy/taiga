/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <array>
#include <string>

namespace sync {

enum class ServiceId {
  Unknown,
  MyAnimeList,
  Kitsu,
  AniList,
};

constexpr std::array<ServiceId, 3> kServiceIds{
  ServiceId::MyAnimeList,
  ServiceId::Kitsu,
  ServiceId::AniList
};

struct Rating {
  int value = 0;
  std::wstring text;
};

ServiceId GetCurrentServiceId();
std::wstring GetCurrentServiceName();
std::wstring GetCurrentServiceSlug();
ServiceId GetServiceIdBySlug(const std::wstring& slug);
std::wstring GetServiceNameById(const ServiceId service_id);
std::wstring GetServiceSlugById(const ServiceId service_id);

}  // namespace sync
