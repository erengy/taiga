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

#include "win_http.h"
#include "base/string.h"

namespace win {
namespace http {

Request::Request()
    : method(L"GET"), parameter(0) {
  // TODO: Generate a UUID
  uuid = L"test-1234567890";
}

Response::Response()
    : code(0), parameter(0) {
}

Client::Client()
    : auto_redirect_(true),
      buffer_(nullptr),
      content_encoding_(kContentEncodingNone),
      content_length_(0),
      current_length_(0),
      connection_handle_(nullptr),
      request_handle_(nullptr),
      session_handle_(nullptr) {
  user_agent_ = L"Mozilla/5.0";
}

Client::~Client() {
  Cleanup();
}

////////////////////////////////////////////////////////////////////////////////

void Client::Cleanup() {
  // Close handles
  if (request_handle_) {
    ::WinHttpSetStatusCallback(request_handle_, nullptr, 0, NULL);
    ::WinHttpCloseHandle(request_handle_);
    request_handle_ = nullptr;
  }
  if (connection_handle_) {
    ::WinHttpCloseHandle(connection_handle_);
    connection_handle_ = nullptr;
  }
  if (session_handle_) {
    ::WinHttpCloseHandle(session_handle_);
    session_handle_ = nullptr;
  }

  // Clear buffer
  if (buffer_) {
    delete [] buffer_;
    buffer_ = nullptr;
  }

  // Reset variables
  content_encoding_ = kContentEncodingNone;
  content_length_ = 0;
  current_length_ = 0;
}

////////////////////////////////////////////////////////////////////////////////

const Request& Client::request() const {
  return request_;
}

const Response& Client::response() const {
  return response_;
}

void Client::set_auto_redirect(bool enabled) {
  auto_redirect_ = enabled;
}

void Client::set_download_path(const std::wstring& download_path) {
  download_path_ = download_path;
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

}  // namespace http
}  // namespace win