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
#include <QString>
#include <optional>
#include <string>

#include "media/anime_list.hpp"

class QJsonObject;
class QUrlQuery;

namespace anime {
enum class SeasonName;
}

namespace sync::kitsu {

QString fromSeasonName(const anime::SeasonName name);

QJsonObject buildLibraryEntryObject(const anime::list::Entry& entry,
                                    const anime::list::Fields dirty, const QString& userId);
QByteArray formUrlEncode(const QUrlQuery& query);
std::optional<int> pagingOffset(const QJsonObject& links, const QString& key);

// Sparse fieldsets
// https://jsonapi.org/format/#fetching-sparse-fieldsets
QString animeFields(const bool minimal = false);
QString libraryEntryFields();
QString userFields();

std::string animePageUrl(const int id);

}  // namespace sync::kitsu
