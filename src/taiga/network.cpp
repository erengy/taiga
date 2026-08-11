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

#include "network.hpp"

#include <QNetworkReply>
#include <QRestReply>

#include "base/string.hpp"
#include "taiga/application.hpp"
#include "taiga/config.h"

namespace taiga {

NetworkAccessManager::NetworkAccessManager(QObject* parent) : QNetworkAccessManager{parent} {
  setAutoDeleteReplies(true);
  setTransferTimeout(std::chrono::seconds{10});

  // @TODO: Set proxy

  connect(this, &QNetworkAccessManager::finished, this, [](QNetworkReply* reply) {
    if (!app()->isDebug()) return;
    qDebug() << "Response status:"
             << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "Response headers:";
    for (const auto& [name, value] : reply->rawHeaderPairs()) {
      qDebug().noquote() << u"%1: %2"_s.arg(name).arg(value);
    }
  });
}

QHttpHeaders NetworkAccessManager::commonHeaders() {
  QHttpHeaders headers;

  static const auto userAgentString = []() {
    return u"%1/%2.%3"_s.arg(TAIGA_APP_NAME).arg(TAIGA_VERSION_MAJOR).arg(TAIGA_VERSION_MINOR);
  };
  headers.append(QHttpHeaders::WellKnownHeader::UserAgent, userAgentString());

  return headers;
}

bool isDdosProtectionActive(const QRestReply& reply) {
  const auto server = reply.networkReply()->rawHeader("Server").toLower();

  switch (reply.httpStatus()) {
    case 403:
      return server.startsWith("ddos-guard");
    case 429:
    case 503:
      return server.startsWith("cloudflare");
    default:
      return false;
  }
}

}  // namespace taiga
