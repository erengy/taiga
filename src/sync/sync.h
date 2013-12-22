/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

#ifndef TAIGA_SYNC_SYNC_H
#define TAIGA_SYNC_SYNC_H

#include "service.h"
#include "base/types.h"
#include "library/history.h"
#include "taiga/http.h"

namespace sync {

void AuthenticateUser();
void GetLibraryEntries();
void GetMetadataById(int id);
void SearchTitle(string_t title);
void Synchronize();
void UpdateLibraryEntry(AnimeValues& anime_values, int id,
                        taiga::HttpClientMode http_client_mode);

void DownloadImage(int id, const std::wstring& image_url);

void AddAuthenticationToRequest(Request& request);
bool RequestNeedsAuthentication(RequestType request_type, ServiceId service_id);

RequestType ClientModeToRequestType(taiga::HttpClientMode client_mode);
taiga::HttpClientMode RequestTypeToClientMode(RequestType request_type);

}  // namespace sync

#endif  // TAIGA_SYNC_SYNC_H