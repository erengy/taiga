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
#include <optional>
#include <string>

class QJsonValue;

namespace base {
class FuzzyDate;
}

namespace anime {
enum class Status;
enum class Type;
struct Details;
}  // namespace anime

namespace anime::list {
enum class Status;
struct Entry;
}

namespace sync::anilist {

base::FuzzyDate parseFuzzyDate(const QJsonValue& json);
anime::list::Status parseListStatus(const QString& value);
float parseScore(int value);
anime::Status parseStatus(const QString& value);
anime::Type parseType(const QString& value);

std::optional<anime::Details> parseMedia(const QJsonValue& json);
std::optional<anime::list::Entry> parseListEntry(const QJsonValue& json);

}  // namespace sync::anilist
