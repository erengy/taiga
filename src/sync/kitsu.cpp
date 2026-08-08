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

#include "kitsu.hpp"

#include <QHttpHeaders>

#include "taiga/accounts.hpp"

// Kitsu API documentation:
// https://kitsu.docs.apiary.io

namespace sync::kitsu {

Service::Service() : sync::Service{ServiceId::Kitsu} {
  api_.setBaseUrl(QUrl{kApiUrl});

  auto headers = api_.commonHeaders();
  headers.append(QHttpHeaders::WellKnownHeader::Accept, kJsonApiMediaType);
  api_.setCommonHeaders(headers);

  if (const auto token = taiga::accounts.kitsuAccessToken(); !token.empty()) {
    api_.setBearerToken(QByteArray::fromStdString(token));
  }
}

Service* Service::instance() {
  static auto service = new Service();
  return service;
}

}  // namespace sync::kitsu
