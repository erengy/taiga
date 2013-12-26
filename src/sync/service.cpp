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

#include "service.h"

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

void Service::BuildRequest(Request& request, HttpRequest& http_request) {
}

void Service::HandleResponse(Response& response, HttpResponse& http_response) {
}

bool Service::RequestNeedsAuthentication(RequestType request_type) const {
  return false;
}

enum_t Service::id() const {
  return id_;
}

string_t Service::canonical_name() const {
  return canonical_name_;
}

string_t Service::name() const {
  return name_;
}

}  // namespace sync