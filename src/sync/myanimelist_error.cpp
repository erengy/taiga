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

#include "myanimelist_error.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QRestReply>
#include <optional>

#include "base/string.hpp"
#include "sync/service.hpp"
#include "taiga/network.hpp"

namespace sync::myanimelist {

namespace {

QString wwwAuthenticateValue(const QRestReply& reply, const QString& key) {
  const auto header = QString::fromUtf8(reply.networkReply()->rawHeader("WWW-Authenticate"));
  return QRegularExpression{key + u"=\"([^\"]*)\""_s}.match(header).captured(1);
}

std::optional<QString> parseErrorMessage(QRestReply& reply) {
  if (const auto description = wwwAuthenticateValue(reply, u"error_description"_s);
      !description.isEmpty()) {
    return description;
  }

  const auto json = reply.readJson();
  if (!json) return std::nullopt;

  const auto root = json->object();

  auto message = root["message"].toString();
  if (const auto hint = root["hint"].toString(); !hint.isEmpty()) {
    message = message.isEmpty() ? hint : u"%1 (%2)"_s.arg(message, hint);
  }

  if (message.isEmpty() && root["error"].toString() == "forbidden") {
    message = "Please set up your account via Settings.";
  }

  if (!message.isEmpty()) return message;

  return std::nullopt;
}

}  // namespace

bool isError(const QRestReply& reply) {
  return !reply.isHttpStatusSuccess() || reply.hasError();
}

bool isTokenExpired(const QRestReply& reply) {
  return wwwAuthenticateValue(reply, u"error"_s) == "invalid_token";
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

}  // namespace sync::myanimelist
