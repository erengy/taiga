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

namespace sync::myanimelist {

constexpr auto kClientId = "f6e398095cf7525360276786ec4407bc";
constexpr auto kRedirectUrl = "https://taiga.moe/api/myanimelist/auth";
constexpr auto kApiUrl = "https://api.myanimelist.net/v2";
constexpr auto kTokenUrl = "https://myanimelist.net/v1/oauth2/token";

class Service final : public sync::Service {
public:
  Service();
  ~Service() = default;

  static Service* instance();

  void authenticateUser();
  void requestAccessToken(const QString& authorizationCode, const QString& codeVerifier);
  void fetchAnime(const int id);
  void fetchListEntries(const int offset = 0);

private:
  void refreshAccessToken(std::function<void()> onSuccess);
  bool retryOnTokenExpiry(QRestReply& reply, std::function<void()> retry);
};

}  // namespace sync::myanimelist
