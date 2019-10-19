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

#include "base/json.h"
#include "base/types.h"
#include "sync/service.h"

namespace sync {
namespace myanimelist {

// API documentation:
// https://myanimelist.net/modules.php?go=api

class Service : public sync::Service {
public:
  Service();
  ~Service() {}

  void BuildRequest(Request& request, HttpRequest& http_request);
  void HandleResponse(Response& response, HttpResponse& http_response);
  bool RequestNeedsAuthentication(RequestType request_type) const;

private:
  REQUEST_AND_RESPONSE(AddLibraryEntry);
  REQUEST_AND_RESPONSE(AuthenticateUser);
  REQUEST_AND_RESPONSE(GetUser);
  REQUEST_AND_RESPONSE(DeleteLibraryEntry);
  REQUEST_AND_RESPONSE(GetLibraryEntries);
  REQUEST_AND_RESPONSE(GetMetadataById);
  REQUEST_AND_RESPONSE(GetSeason);
  REQUEST_AND_RESPONSE(SearchTitle);
  REQUEST_AND_RESPONSE(UpdateLibraryEntry);

  bool RequestSucceeded(Response& response, const HttpResponse& http_response);

  int ParseAnimeObject(const Json& json) const;
  int ParseLibraryObject(const Json& json, int anime_id) const;
  void ParseLinks(const Json& json, Response& response) const;

  bool ParseResponseBody(const std::wstring& body, Response& response, Json& json);

  std::wstring Service::GetAnimeFields() const;
  std::wstring Service::GetListStatusFields() const;
};

}  // namespace myanimelist
}  // namespace sync
