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
#include <QRestReply>

#include "anilist.hpp"
#include "sync/anilist/anilist_error.hpp"
#include "sync/anilist/anilist_utils.hpp"
#include "taiga/accounts.hpp"

namespace sync::anilist {

void Service::authenticateUser() {
  const QJsonDocument data{QJsonObject{
      {"query", gql("Viewer")},
  }};

  const auto callback = [this](QRestReply& reply) {
    if (isError(reply)) {
      handleError(*this, reply);
      emit authenticationCompleted(false);
      return;
    }

    const auto viewer = reply.readJson().and_then([](const QJsonDocument& json) {
      return std::make_optional(json["data"]["Viewer"].toObject());
    });

    if (!viewer) {
      handleError(*this, reply, "Could not parse user object.");
      emit authenticationCompleted(false);
      return;
    }

    taiga::accounts.setAnilistUsername((*viewer)["name"].toString().toStdString());
    taiga::accounts.setAnilistRatingSystem(
        (*viewer)["mediaListOptions"]["scoreFormat"].toString().toStdString());

    emit authenticationCompleted(true);
  };

  manager_.post(api_.createRequest(), data, this, callback);
}

}  // namespace sync::anilist
