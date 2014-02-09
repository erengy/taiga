/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#include "base/file.h"
#include "base/logger.h"
#include "base/string.h"

namespace win {
namespace http {

Request::Request()
    : method(L"GET"), parameter(0), protocol(kHttp) {
  // Each HTTP request must have a unique ID, as there are many parts of the
  // application that rely on this assumption.
  // TODO: Generate a real UUID
  static unsigned int counter = 0;
  uuid = L"win-http-" + PadChar(ToWstr(static_cast<ULONG>(counter++)), L'0', 10);
  LOG(LevelDebug, L"UUID: " + uuid);
}

Response::Response()
    : code(0), parameter(0) {
}

void Request::Clear() {
  protocol = kHttp;
  method = L"GET";
  host.clear();
  path.clear();
  query.clear();
  header.clear();
  body.clear();
}

void Response::Clear() {
  code = 0;
  header.clear();
  body.clear();
}

Client::Client()
    : auto_redirect_(true),
      buffer_(nullptr),
      content_encoding_(kContentEncodingNone),
      content_length_(0),
      current_length_(0),
      connection_handle_(nullptr),
      request_handle_(nullptr),
      secure_transaction_(false),
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

  // Clear request and response
  request_.Clear();
  response_.Clear();

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

DWORD Client::content_length() const {
  return content_length_;
}

DWORD Client::current_length() const {
  return current_length_;
}

void Client::set_auto_redirect(bool enabled) {
  auto_redirect_ = enabled;
}

void Client::set_download_path(const std::wstring& download_path) {
  download_path_ = download_path;

  // Make sure the path is available
  if (!download_path.empty())
    CreateFolder(GetPathOnly(download_path));
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