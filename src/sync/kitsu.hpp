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

#include <functional>

#include "sync/service.hpp"

class QRestReply;

namespace sync::kitsu {

// Application registration has not yet been implemented on Kitsu's end, so all requests are made
// with the following public client ID and secret.
constexpr auto kClientId = "dd031b32d2f56c990b1425efe6c42ad847e7fe3ab46bf1299f05ecd856bdb7dd";
constexpr auto kClientSecret = "54d7307928f63414defd96399fc31ba847961ceaecef3a5fd93144e960c0e151";
constexpr auto kApiUrl = "https://kitsu.app/api/edge";
constexpr auto kTokenUrl = "https://kitsu.app/api/oauth/token";
constexpr auto kJsonApiMediaType = "application/vnd.api+json";

enum ListPrivacy {
  kPrivate = 1,
  kPublic,
};

enum ListStatus {
  kCurrentlyWatching = 1,
  kPlanToWatch,
  kCompleted,
  kOnHold,
  kDropped,
};

class Service final : public sync::Service {
public:
  Service();
  ~Service() = default;

  static Service* instance();

  void authenticateUser();
  void fetchAnime(const int id);
  void fetchListEntries(const int offset = 0);
  void search(const SearchParams& params, const int offset = 0);
  void addListEntry(const int id, const anime::list::Fields dirty);
  void updateListEntry(const int id, const anime::list::Fields dirty);
  void deleteListEntry(const int id);

private:
  void resolveUser(std::function<void()> onSuccess = nullptr);
  void refreshAccessToken(std::function<void()> onSuccess);
  bool retryOnTokenExpiry(QRestReply& reply, std::function<void()> retry);
};

}  // namespace sync::kitsu
