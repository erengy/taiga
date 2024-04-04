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

#include <string>

namespace anime {
class Season;
}
namespace library {
struct QueueItem;
}

namespace sync::myanimelist {

constexpr auto kClientId = "f6e398095cf7525360276786ec4407bc";
constexpr auto kRedirectUrl = "https://taiga.moe/api/myanimelist/auth";

void RequestAccessToken(const std::wstring& authorization_code,
                        const std::wstring& code_verifier);
void RefreshAccessToken();
void GetUser();
void GetLibraryEntries(const int page_offset = 0);
void GetMetadataById(const int id);
void GetSeason(const anime::Season season, const int page_offset = 0);
void SearchTitle(const std::wstring& title);
void AddLibraryEntry(const library::QueueItem& queue_item);
void DeleteLibraryEntry(const int id);
void UpdateLibraryEntry(const library::QueueItem& queue_item);

bool IsUserAuthenticated();
void InvalidateUserAuthentication();

}  // namespace sync::myanimelist
