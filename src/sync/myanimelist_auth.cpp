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

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QRestReply>
#include <QUrlQuery>

#include "base/string.hpp"
#include "myanimelist.hpp"
#include "sync/myanimelist_error.hpp"
#include "sync/myanimelist_utils.hpp"
#include "taiga/accounts.hpp"

namespace sync::myanimelist {

bool Service::retryOnTokenExpiry(QRestReply& reply, std::function<void()> retry) {
  if (!isTokenExpired(reply)) return false;
  refreshAccessToken(std::move(retry));
  return true;
}

void Service::requestAccessToken(const QString& authorizationCode, const QString& codeVerifier) {
  QNetworkRequest request{QUrl{kTokenUrl}};
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

  const QUrlQuery body{{
      {"client_id", kClientId},
      {"grant_type", "authorization_code"},
      {"code", authorizationCode},
      {"redirect_uri", kRedirectUrl},
      {"code_verifier", codeVerifier},
  }};

  const auto callback = [this](QRestReply& reply) {
    if (isError(reply)) {
      handleError(*this, reply);
      emit authenticationCompleted(false);
      return;
    }

    const auto json = reply.readJson();
    if (!json) {
      handleError(*this, reply, "Could not parse authentication data.");
      emit authenticationCompleted(false);
      return;
    }

    const auto root = json->object();
    const auto accessToken = root["access_token"].toString();
    taiga::accounts.setMyanimelistAccessToken(accessToken.toStdString());
    taiga::accounts.setMyanimelistRefreshToken(root["refresh_token"].toString().toStdString());
    api_.setBearerToken(accessToken.toUtf8());

    authenticateUser();
  };

  manager_.post(request, formUrlEncode(body), this, callback);
}

void Service::refreshAccessToken(std::function<void()> onSuccess) {
  const auto refreshToken = taiga::accounts.myanimelistRefreshToken();

  if (refreshToken.empty()) {
    emit errorOccurred("Refresh token is unavailable.");
    emit authenticationCompleted(false);
    return;
  }

  QNetworkRequest request{QUrl{kTokenUrl}};
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

  const QUrlQuery body{{
      {"client_id", kClientId},
      {"grant_type", "refresh_token"},
      {"refresh_token", QString::fromStdString(refreshToken)},
  }};

  const auto callback = [this, onSuccess](QRestReply& reply) {
    if (isError(reply)) {
      handleError(*this, reply);
      emit authenticationCompleted(false);
      return;
    }

    const auto json = reply.readJson();
    if (!json) {
      handleError(*this, reply, "Could not parse authentication data.");
      emit authenticationCompleted(false);
      return;
    }

    const auto root = json->object();
    const auto accessToken = root["access_token"].toString();
    taiga::accounts.setMyanimelistAccessToken(accessToken.toStdString());
    taiga::accounts.setMyanimelistRefreshToken(root["refresh_token"].toString().toStdString());
    api_.setBearerToken(accessToken.toUtf8());

    if (onSuccess) onSuccess();
  };

  manager_.post(request, formUrlEncode(body), this, callback);
}

////////////////////////////////////////////////////////////////////////////////

void Service::authenticateUser() {
  const auto callback = [this](QRestReply& reply) {
    if (isError(reply)) {
      if (retryOnTokenExpiry(reply, [this] { authenticateUser(); })) return;
      handleError(*this, reply);
      emit authenticationCompleted(false);
      return;
    }

    const auto json = reply.readJson();
    if (!json) {
      handleError(*this, reply, "Could not parse user object.");
      emit authenticationCompleted(false);
      return;
    }

    taiga::accounts.setMyanimelistUsername(json->object()["name"].toString().toStdString());

    emit authenticationCompleted(true);
  };

  manager_.get(api_.createRequest(u"/users/@me"_s), this, callback);
}

}  // namespace sync::myanimelist
