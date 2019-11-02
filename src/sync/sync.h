/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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
namespace hypr::detail {
struct Transfer;
}
namespace library {
struct QueueItem;
}

namespace sync {

bool AuthenticateUser();
void GetUser();
void GetLibraryEntries();
void GetMetadataById(const int id);
void GetSeason(const anime::Season season);
void SearchTitle(const std::wstring& title, const int id);
void Synchronize();
void UpdateLibraryEntry(const library::QueueItem& queue_item);

void DownloadImage(const int anime_id, const std::wstring& image_url);

bool UserAuthenticated();
void InvalidateUserAuthentication();
bool IsUserAccountAvailable();

void AfterGetLibrary();
void AfterGetSeason();
void AfterLibraryUpdate();

bool OnTransfer(const hypr::detail::Transfer& transfer,
                const std::wstring& status);

}  // namespace sync
