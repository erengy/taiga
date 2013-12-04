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
#include "base/foreach.h"
#include "base/string.h"

namespace win {
namespace http {

bool Client::MakeRequest(Request request) {
  Cleanup();  // Close any previous connection

  // Set the new request
  request_ = request;

  // Ensure that the response has the same parameter and UUID as the request
  response_.parameter = request.parameter;
  response_.uuid = request.uuid;

  if (OpenSession() && ConnectToSession())
    if (OpenRequest() && SetRequestOptions())
      if (SendRequest())
        return true;

  Cleanup();
  OnError(::GetLastError());
  return false;
}

////////////////////////////////////////////////////////////////////////////////

HINTERNET Client::OpenSession() {
  // Initialize the use of WinHTTP functions for the application
  session_handle_ = ::WinHttpOpen(user_agent_.c_str(),
                                  WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                  WINHTTP_NO_PROXY_NAME,
                                  WINHTTP_NO_PROXY_BYPASS,
                                  WINHTTP_FLAG_ASYNC);
  return session_handle_;
}

HINTERNET Client::ConnectToSession() {
  // Specify the initial target server of an HTTP request
  connection_handle_ = ::WinHttpConnect(session_handle_,
                                        request_.host.c_str(),
                                        INTERNET_DEFAULT_HTTP_PORT,  // port 80
                                        0);  // reserved, must be 0
  return connection_handle_;
}

HINTERNET Client::OpenRequest() {
  // Append the query component to the path
  std::wstring path = request_.path;
  if (!request_.query.empty()) {
    std::wstring query_string;
    foreach_(it, request_.query) {
      query_string += query_string.empty() ? L"?" : L"&";
      query_string += it->first + L"=" + GetUrlEncodedString(it->second, false);
    }
    path += query_string;
  }

  // Create an HTTP request handle
  request_handle_ = ::WinHttpOpenRequest(connection_handle_,
                                         request_.method.c_str(),
                                         path.c_str(),
                                         nullptr,  // use HTTP/1.1
                                         referer_.c_str(),
                                         WINHTTP_DEFAULT_ACCEPT_TYPES,
                                         0);  // no flags set
  return request_handle_;
}

BOOL Client::SetRequestOptions() {
  // Set proxy information
  if (!proxy_host_.empty()) {
    WINHTTP_PROXY_INFO pi = {0};
    pi.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
    pi.lpszProxy = const_cast<LPWSTR>(proxy_host_.c_str());
    ::WinHttpSetOption(request_handle_, WINHTTP_OPTION_PROXY,
                       &pi, sizeof(pi));
    if (!proxy_username_.empty()) {
      ::WinHttpSetOption(request_handle_,
                         WINHTTP_OPTION_PROXY_USERNAME,
                         (LPVOID)proxy_username_.c_str(),
                         proxy_username_.size() * sizeof(wchar_t));
      if (!proxy_password_.empty()) {
        ::WinHttpSetOption(request_handle_,
                           WINHTTP_OPTION_PROXY_PASSWORD,
                           (LPVOID)proxy_password_.c_str(),
                           proxy_password_.size() * sizeof(wchar_t));
      }
    }
  }

  // Set auto-redirect
  if (!auto_redirect_) {
    DWORD option = WINHTTP_DISABLE_REDIRECTS;
    ::WinHttpSetOption(request_handle_, WINHTTP_OPTION_DISABLE_FEATURE,
                       &option, sizeof(option));
  }

  // Set callback function
  WINHTTP_STATUS_CALLBACK callback = ::WinHttpSetStatusCallback(
      request_handle_,
      reinterpret_cast<WINHTTP_STATUS_CALLBACK>(&Callback),
      WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,
      NULL);  // reserved, must be null
  return callback != WINHTTP_INVALID_STATUS_CALLBACK;
}

BOOL Client::SendRequest() {
  std::wstring header = BuildRequestHeader();

  // This buffer must remain available until the request handle is closed or the
  // call to WinHttpReceiveResponse has completed
  optional_data_ = WstrToStr(request_.body);
  LPVOID optional_data = optional_data_.empty() ?
      WINHTTP_NO_REQUEST_DATA : (LPVOID)optional_data_.c_str();

  // Send the request to the HTTP server
  return ::WinHttpSendRequest(request_handle_,
                              header.c_str(),
                              header.length(),
                              optional_data,
                              optional_data_.length(),
                              optional_data_.length(),
                              reinterpret_cast<DWORD_PTR>(this));
}

////////////////////////////////////////////////////////////////////////////////

std::wstring Client::BuildRequestHeader() {
  // Set acceptable types for the response
  if (!request_.header.count(L"Accept"))
    request_.header[L"Accept"] = L"*/*";

  // Set content type for POST and PUT requests
  if (request_.method == L"POST" || request_.method == L"PUT")
    if (!request_.header.count(L"Content-Type"))
      request_.header[L"Content-Type"] = L"application/x-www-form-urlencoded";

  std::wstring header;

  // Append available header fields
  foreach_(it, request_.header)
    header += it->first + L": " + it->second + L"\r\n";

  return header;
}

}  // namespace http
}  // namespace win