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

#include "myanimelist_utils.hpp"

#include <QJsonObject>
#include <QRandomGenerator>
#include <QUrl>
#include <QUrlQuery>
#include <algorithm>
#include <format>

#include "base/string.hpp"
#include "media/anime_list.hpp"
#include "media/anime_season.hpp"
#include "sync/myanimelist/myanimelist.hpp"

namespace sync::myanimelist {

int fromListScore(int value) {
  return (value * 10) / anime::list::kScoreMax;
}

QString fromListStatus(const anime::list::Status value) {
  // clang-format off
  switch (value) {
    case anime::list::Status::Watching: return "watching";
    case anime::list::Status::Completed: return "completed";
    case anime::list::Status::OnHold: return "on_hold";
    case anime::list::Status::Dropped: return "dropped";
    case anime::list::Status::PlanToWatch: return "plan_to_watch";
  }
  // clang-format on
  return "watching";
}

QString fromSeasonName(const anime::SeasonName name) {
  // clang-format off
  switch (name) {
    case anime::SeasonName::Unknown: return "";
    case anime::SeasonName::Winter: return "winter";
    case anime::SeasonName::Spring: return "spring";
    case anime::SeasonName::Summer: return "summer";
    case anime::SeasonName::Fall: return "fall";
  }
  // clang-format on
  return "";
}

////////////////////////////////////////////////////////////////////////////////

QString animeFields() {
  return u"alternative_titles,"
         "average_episode_duration,"
         "end_date,"
         "genres,"
         "id,"
         "main_picture,"
         "mean,"
         "media_type,"
         "num_episodes,"
         "popularity,"
         "rating,"
         "start_date,"
         "status,"
         "studios,"
         "synopsis,"
         "title"_s;
}

QString listStatusFields() {
  return u"comments,"
         "finish_date,"
         "is_rewatching,"
         "num_times_rewatched,"
         "num_watched_episodes,"
         "score,"
         "start_date,"
         "status,"
         "updated_at"_s;
}

QByteArray formUrlEncode(const QUrlQuery& query) {
  return query.toString(QUrl::FullyEncoded).toUtf8();
}

std::optional<int> pagingOffset(const QJsonObject& paging, const QString& key) {
  const QUrlQuery query{QUrl{paging[key].toString()}};

  if (!query.hasQueryItem(u"offset"_s)) return std::nullopt;

  bool ok = false;
  const auto offset = query.queryItemValue(u"offset"_s).toInt(&ok);
  return ok ? std::make_optional(offset) : std::nullopt;
}

////////////////////////////////////////////////////////////////////////////////

std::string animePageUrl(const int id) {
  return std::format("https://myanimelist.net/anime/{}/", id);
}

std::string authorizationCodeUrl(std::string& codeVerifier) {
  codeVerifier.assign(64, '\0');
  std::generate(codeVerifier.begin(), codeVerifier.end(), []() {
    static const std::string unreserved =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstvuwxyz"
        "0123456789-._~";
    return unreserved[QRandomGenerator::global()->bounded(unreserved.size())];
  });

  QUrl url{"https://myanimelist.net/v1/oauth2/authorize"};
  url.setQuery({
      {"response_type", "code"},
      {"client_id", kClientId},
      {"redirect_uri", kRedirectUrl},
      {"code_challenge", QString::fromStdString(codeVerifier)},
      {"code_challenge_method", "plain"},
  });
  return url.toString().toStdString();
}

}  // namespace sync::myanimelist
