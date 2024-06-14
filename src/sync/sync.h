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
namespace hypr::detail {
struct Transfer;
}
namespace library {
struct QueueItem;
}
namespace taiga::http {
using Transfer = hypr::detail::Transfer;
}

namespace sync {

enum class RequestType {
  RequestAccessToken,
  RefreshAccessToken,
  AuthenticateUser,
  GetUser,
  GetLibraryEntries,
  GetMetadataById,
  GetSeason,
  SearchTitle,
  AddLibraryEntry,
  DeleteLibraryEntry,
  UpdateLibraryEntry,
};

void AuthenticateUser();
void GetUser();
void GetLibraryEntries();
void GetMetadataById(const int id);
void GetSeason(const anime::Season season);
void SearchTitle(const std::wstring& title);
void Synchronize();
void AddLibraryEntry(const library::QueueItem& queue_item);
void DeleteLibraryEntry(const int id);
void UpdateLibraryEntry(const library::QueueItem& queue_item);

void DownloadImage(const int anime_id, const std::wstring& image_url);

bool IsUserAuthenticated();
void InvalidateUserAuthentication();
bool IsUserAccountAvailable();
bool IsUserAuthenticationAvailable();

void OnError(const RequestType type);
bool OnTransfer(const RequestType type, const taiga::http::Transfer& transfer,
                const std::wstring& status);
void OnResponse(const RequestType type);

void OnInvalidAnimeId(const int id);

}  // namespace sync
