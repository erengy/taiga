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

#include "base/types.h"

// A service, in Taiga's terms, is a web application that provides an API that
// is based on HTTP requests and responses. Services are used for metadata
// retrieval and library synchronization. At least one service must be enabled
// for Taiga to function properly.

namespace sync {

enum ServiceId {
  kAllServices = 0,
  kTaiga = 0,
  kFirstService = 1,
  kMyAnimeList = 1,
  kKitsu = 2,
  kAniList = 3,
  kLastService = 3
};

enum RequestType {
  kGenericRequest,
  kAuthenticateUser,
  kGetUser,
  kGetMetadataById,
  kGetSeason,
  kSearchTitle,
  kAddLibraryEntry,
  kDeleteLibraryEntry,
  kGetLibraryEntries,
  kUpdateLibraryEntry
};

class Request {
public:
  Request();
  Request(RequestType type);
  virtual ~Request() {}

  ServiceId service_id;
  RequestType type;
  dictionary_t data;
};

class Response {
public:
  Response();
  virtual ~Response() {}

  ServiceId service_id;
  RequestType type;
  dictionary_t data;
};

struct User {
  std::wstring id;
  std::wstring username;
  std::wstring rating_system;

  std::wstring access_token;
  std::wstring refresh_token;
  bool authenticated = false;
  time_t last_synchronized = 0;
};

struct Rating {
  int value = 0;
  std::wstring text;
};

class Service {
public:
  Service();
  virtual ~Service() {}

  virtual void BuildRequest(Request& request, HttpRequest& http_request) = 0;
  virtual void HandleResponse(Response& response, HttpResponse& http_response) = 0;
  virtual bool RequestNeedsAuthentication(RequestType request_type) const;

  const std::wstring& host() const;
  enum_t id() const;
  const std::wstring& canonical_name() const;
  const std::wstring& name() const;

  User& user();
  const User& user() const;

protected:
  void HandleError(const HttpResponse& http_response, Response& response) const;

  // API end-point
  std::wstring host_;
  // Service identifiers
  enum_t id_;
  std::wstring canonical_name_;
  std::wstring name_;
  // User information
  User user_;
};

// Creates two overloaded functions: First one is to build a request, second
// one is to handle a response.
#define REQUEST_AND_RESPONSE(f) \
    void f(Request& request, HttpRequest& http_request); \
    void f(Response& response, HttpResponse& http_response);
// Other helper macros
#define BUILD_HTTP_REQUEST(type, function) \
    case type: function(request, http_request); break;
#define HANDLE_HTTP_RESPONSE(type, function) \
    case type: function(response, http_response); break;

}  // namespace sync
