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

#include <QByteArray>
#include <QList>
#include <QString>
#include <optional>
#include <string>

class QJsonObject;
class QUrlQuery;

namespace anime {
enum class SeasonName;
}

namespace anime::list {
enum class Status;
}

namespace sync {
struct Rating;
}

namespace sync::myanimelist {

int fromListScore(int value);
QString fromListStatus(const anime::list::Status value);
QString fromSeasonName(const anime::SeasonName name);

QList<sync::Rating> ratingList();
QString formatRating(const int value);

QString animeFields();
QString listStatusFields();
QByteArray formUrlEncode(const QUrlQuery& query);
std::optional<int> pagingOffset(const QJsonObject& paging, const QString& key);

std::string animePageUrl(const int id);
std::string authorizationCodeUrl(std::string& codeVerifier);

}  // namespace sync::myanimelist
