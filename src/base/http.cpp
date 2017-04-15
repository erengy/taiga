/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#include "file.h"
#include "http.h"
#include "string.h"

namespace base {
namespace http {

Request::Request()
    : method(L"GET"), parameter(0), uid(GenerateRequestId()) {
}

Response::Response()
    : code(0), parameter(0) {
}

void Request::Clear() {
  method = L"GET";
  url.Clear();
  header.clear();
  body.clear();
  data.clear();
}

void Response::Clear() {
  code = 0;
  header.clear();
  body.clear();
}

unsigned int Response::GetStatusCategory() const {
  return code - (code % 100);
}

std::wstring GenerateRequestId() {
  // Each HTTP request must have a unique ID, as there are many parts of the
  // application that rely on this assumption.
  static unsigned int counter = 0;
  return L"taiga-http-" + PadChar(ToWstr(counter++), L'0', 10);
}

Client::Client(const Request& request)
    : allow_reuse_(false),
      auto_redirect_(true),
      busy_(false),
      cancel_(false),
      content_encoding_(ContentEncoding::None),
      content_length_(0),
      current_length_(0),
      curl_handle_(nullptr),
      debug_mode_(false),
      header_list_(nullptr),
      no_revoke_(false),
      request_(request),
      user_agent_(L"Mozilla/5.0") {
}

Client::~Client() {
  Cleanup(false);
}

////////////////////////////////////////////////////////////////////////////////

void Client::Cancel() {
  cancel_ = true;
}

void Client::Cleanup(bool reuse) {
  // Close handles
  if (curl_handle_) {
    if (reuse) {
      curl_easy_reset(curl_handle_);
    } else {
      curl_easy_cleanup(curl_handle_);
      curl_handle_ = nullptr;
    }
  }
  if (header_list_) {
    curl_slist_free_all(header_list_);
    header_list_ = nullptr;
  }
  if (GetThreadHandle() &&
      GetThreadId() != GetCurrentThreadId()) {
    CloseThreadHandle();
  }

  // Clear request and response
  if (!reuse)
    request_.Clear();
  response_.Clear();

  // Clear buffers
  optional_data_.clear();
  write_buffer_.clear();

  // Reset variables
  busy_ = false;
  cancel_ = false;
  content_encoding_ = ContentEncoding::None;
  content_length_ = 0;
  current_length_ = 0;
}

////////////////////////////////////////////////////////////////////////////////

bool Client::allow_reuse() const {
  return allow_reuse_;
}

bool Client::busy() const {
  return busy_;
}

bool Client::no_revoke() const {
  return no_revoke_;
}

const Request& Client::request() const {
  return request_;
}

const Response& Client::response() const {
  return response_;
}

curl_off_t Client::content_length() const {
  return content_length_;
}

curl_off_t Client::current_length() const {
  return current_length_;
}

void Client::set_allow_reuse(bool allow) {
  allow_reuse_ = allow;
}

void Client::set_auto_redirect(bool enabled) {
  auto_redirect_ = enabled;
}

void Client::set_debug_mode(bool enabled) {
  debug_mode_ = enabled;
}

void Client::set_no_revoke(bool enabled) {
  no_revoke_ = enabled;
}

void Client::set_proxy(const std::wstring& host,
                       const std::wstring& username,
                       const std::wstring& password) {
  proxy_host_ = host;
  proxy_username_ = username;
  proxy_password_ = password;
}

void Client::set_referer(const std::wstring& referer) {
  referer_ = referer;
}

void Client::set_user_agent(const std::wstring& user_agent) {
  user_agent_ = user_agent;
}

////////////////////////////////////////////////////////////////////////////////

CurlGlobal Client::curl_global_;

CurlGlobal::CurlGlobal()
    : initialized_(false) {
  if (curl_global_init(CURL_GLOBAL_ALL) == 0)
    initialized_ = true;
}

CurlGlobal::~CurlGlobal() {
  if (initialized_) {
    curl_global_cleanup();
    initialized_ = false;
  }
}

bool CurlGlobal::initialized() const {
  return initialized_;
}

}  // namespace http
}  // namespace base