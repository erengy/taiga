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

#include "anilist_utils.hpp"

#include <QJsonObject>
#include <QString>
#include <QUrl>
#include <QUrlQuery>

#include "base/chrono.hpp"
#include "base/file.hpp"
#include "base/string.hpp"
#include "media/anime.hpp"
#include "media/anime_list.hpp"
#include "media/anime_season.hpp"

namespace sync::anilist {

QJsonObject fromFuzzyDate(const FuzzyDate& date) {
  return {
      {"year", date.year()},
      {"month", date.month()},
      {"day", date.day()},
  };
}

QString fromListStatus(const anime::list::Status value) {
  // clang-format off
  switch (value) {
    case anime::list::Status::Watching: return "CURRENT";
    case anime::list::Status::Completed: return "COMPLETED";
    case anime::list::Status::OnHold: return "PAUSED";
    case anime::list::Status::Dropped: return "DROPPED";
    case anime::list::Status::PlanToWatch: return "PLANNING";
  }
  // clang-format on
  return "";
}

float fromScore(float value) {
  return value * 10.0f;
}

QString fromSeasonName(const anime::SeasonName name) {
  // clang-format off
  switch (name) {
    case anime::SeasonName::Unknown: return "";
    case anime::SeasonName::Winter: return "WINTER";
    case anime::SeasonName::Spring: return "SPRING";
    case anime::SeasonName::Summer: return "SUMMER";
    case anime::SeasonName::Fall: return "FALL";
  }
  // clang-format on
  return "";
}

////////////////////////////////////////////////////////////////////////////////

QString gql(const QString& name) {
  const auto readGql = [](const QString& name) {
    return base::readFile(u":/gql/anilist/%1.gql"_s.arg(name));
  };

  auto query = readGql(name);

  for (const auto& fragment : {u"MediaFragment"_s, u"MediaListFragment"_s}) {
    if (query.contains(u"...%1"_s.arg(fragment))) {
      query += u"\n%1"_s.arg(readGql(fragment));
    }
  }

  return query;
}

////////////////////////////////////////////////////////////////////////////////

std::string animePageUrl(const int id) {
  return std::format("https://anilist.co/anime/{}", id);
}

std::string requestTokenUrl() {
  constexpr auto kTaigaClientId = 161;
  QUrl url{"https://anilist.co/api/v2/oauth/authorize"};
  url.setQuery({
      {"client_id", QString::number(kTaigaClientId)},
      {"response_type", "token"},
  });
  return url.toString().toStdString();
}

}  // namespace sync::anilist
