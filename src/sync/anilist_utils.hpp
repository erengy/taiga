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

#pragma once

#include <QString>
#include <string>

class QJsonObject;

namespace base {
class FuzzyDate;
}

namespace anime {
enum class SeasonName;
}

namespace anime::list {
enum class Status;
}

namespace sync::anilist {

QJsonObject fromFuzzyDate(const base::FuzzyDate& date);
QString fromListStatus(const anime::list::Status value);
float fromScore(float value);
QString fromSeasonName(const anime::SeasonName name);

QString gql(const QString& name);

std::string animePageUrl(const int id);
std::string requestTokenUrl();

}  // namespace sync::anilist
