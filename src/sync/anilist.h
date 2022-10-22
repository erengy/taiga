/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>

namespace anime {
class Season;
}
namespace library {
struct QueueItem;
}

namespace sync::anilist {

void AuthenticateUser();
void GetLibraryEntries();
void GetMetadataById(const int id);
void GetSeason(const anime::Season season, const int page = 1);
void SearchTitle(const std::wstring& title);
void AddLibraryEntry(const library::QueueItem& queue_item);
void DeleteLibraryEntry(const int id);
void UpdateLibraryEntry(const library::QueueItem& queue_item);

bool IsUserAuthenticated();
void InvalidateUserAuthentication();

}  // namespace sync::anilist
