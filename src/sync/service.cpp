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

#include "base/format.h"
#include "base/http.h"
#include "sync/service.h"

namespace sync {

Request::Request()
    : service_id(kAllServices), type(kGenericRequest) {
}

Request::Request(RequestType type)
    : service_id(kAllServices), type(type) {
}

Response::Response()
    : service_id(kAllServices), type(kGenericRequest) {
}

////////////////////////////////////////////////////////////////////////////////

Service::Service()
    : id_(0) {
}

bool Service::RequestNeedsAuthentication(RequestType request_type) const {
  return false;
}

const string_t& Service::host() const {
  return host_;
}

enum_t Service::id() const {
  return id_;
}

const string_t& Service::canonical_name() const {
  return canonical_name_;
}

const string_t& Service::name() const {
  return name_;
}

User& Service::user() {
  return user_;
}

const User& Service::user() const {
  return user_;
}

////////////////////////////////////////////////////////////////////////////////

void Service::HandleError(const HttpResponse& http_response,
                          Response& response) const {
  auto& error = response.data[L"error"];

  if (!error.empty()) {
    error = L"{} returned an error: {}"_format(name(), error);
  } else {
    error = L"{} returned an unknown error. "
            L"Request type: {} - HTTP status code: {}"_format(
            name(), response.type, http_response.code);
  }
}

}  // namespace sync
