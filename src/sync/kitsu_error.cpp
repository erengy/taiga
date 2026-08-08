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

#include "kitsu_error.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRestReply>
#include <optional>

#include "base/string.hpp"
#include "sync/service.hpp"
#include "taiga/network.hpp"

namespace sync::kitsu {

namespace {

std::optional<QString> parseErrorMessage(QRestReply& reply) {
  const auto json = reply.readJson();
  if (!json) return std::nullopt;

  const auto root = json->object();

  // OAuth token endpoint errors
  // {"error": "...", "error_description": "..."}
  if (const auto description = root["error_description"].toString(); !description.isEmpty()) {
    return description;
  }

  // JSON:API errors
  // {"errors": [{"title": "...", "detail": "..."}]}
  const auto errors = root["errors"].toArray();
  if (!errors.isEmpty()) {
    const auto error = errors.first().toObject();
    const auto title = error["title"].toString();
    const auto detail = error["detail"].toString();
    if (!title.isEmpty() && !detail.isEmpty()) return u"%1: %2"_s.arg(title, detail);
    if (!detail.isEmpty()) return detail;
    if (!title.isEmpty()) return title;
  }

  return std::nullopt;
}

}  // namespace

bool isError(const QRestReply& reply) {
  return !reply.isHttpStatusSuccess() || reply.hasError();
}

bool isTokenExpired(const QRestReply& reply) {
  return reply.httpStatus() == 401;
}

void handleError(sync::Service& service, QRestReply& reply, const QString& message) {
  if (taiga::isDdosProtectionActive(reply)) {
    const auto server = QString::fromUtf8(reply.networkReply()->rawHeader("Server"));
    const auto description =
        u"Cannot connect to server because of DDoS protection (Server: %1)"_s.arg(server);
    emit service.errorOccurred(description);
    return;
  }

  if (const auto description = parseErrorMessage(reply)) {
    emit service.errorOccurred(*description);
    return;
  }

  if (!message.isEmpty()) {
    emit service.errorOccurred(message);
    return;
  }

  if (reply.hasError()) {
    emit service.errorOccurred(reply.errorString());
  }
}

}  // namespace sync::kitsu
