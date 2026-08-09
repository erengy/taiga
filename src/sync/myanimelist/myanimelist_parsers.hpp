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

#include <QString>
#include <optional>

class QJsonValue;

namespace base {
class FuzzyDate;
}

namespace anime {
enum class AgeRating;
enum class Status;
enum class Type;
struct Details;
}  // namespace anime

namespace anime::list {
enum class Status;
struct Entry;
}  // namespace anime::list

namespace sync::myanimelist {

anime::AgeRating parseAgeRating(const QString& value);
base::FuzzyDate parseFuzzyDate(const QString& value);
int parseEpisodeLength(int value);
anime::Status parseStatus(const QString& value);
anime::Type parseType(const QString& value);
std::time_t parseListLastUpdated(const QString& value);
int parseListScore(int value);
anime::list::Status parseListStatus(const QString& value);

std::optional<anime::Details> parseAnime(const QJsonValue& json);
std::optional<anime::list::Entry> parseListEntry(const QJsonValue& json, const int animeId);

}  // namespace sync::myanimelist
