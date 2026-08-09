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

#include "anilist_error.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRestReply>
#include <optional>

#include "base/string.hpp"
#include "sync/service.hpp"
#include "taiga/network.hpp"

namespace sync::anilist {

namespace {

std::optional<QString> parseErrorMessage(QRestReply& reply) {
  const auto json = reply.readJson();
  if (!json) return std::nullopt;

  for (const auto& value : (*json)["errors"].toArray()) {
    const auto error = value.toObject();
    auto message = error["message"].toString();

    // Rewrite validation errors
    if (const auto validation = error["validation"].toObject(); !validation.isEmpty()) {
      const auto messages = validation.constBegin().value().toArray();
      if (!messages.isEmpty()) {
        message = u"Validation error: %1"_s.arg(messages.first().toString());
      }
    }

    // Rewrite access token error
    if (message == "Invalid token") {
      message = "Access token has expired. Please re-authorize your account via Settings.";
    }

    // Return the first non-empty message
    if (!message.isEmpty()) return message;
  }

  return std::nullopt;
}

}  // namespace

bool isError(const QRestReply& reply) {
  return !reply.isHttpStatusSuccess() || reply.hasError();
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

}  // namespace sync::anilist
